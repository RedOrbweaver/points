#pragma once

#include "hmain.hpp"

// Handles point processing as well as file loading
// All heavy operations are done in a separate thread
// Certain functions require using the Lock() and Unlock() functions to ensure thread safety
class PointProcessor
{
    protected:
    // Assuming most of the loading time will be spent fetching data from disk and parsing it
    constexpr static const float WEIGHT_PARSE = 0.7f;
    constexpr static const float WEIGHT_COMPUTE = 0.1f;
    // Assuming 7 characters per line + 2 spaces
    constexpr static const int ASSUMED_BYTES_PER_LINE = 23;

    std::string file_name;
    std::string path;
    size_t file_size;
    size_t memory_used;
    // sorted in ascending order
    std::vector<vec3<float>> points;
    std::vector<float> sections;
    std::vector<size_t> section_indices;
    std::mutex access_mx;
    std::mutex notify_mx;
    std::thread processing_thread;
    std::condition_variable process_notify;
    bool is_loaded = false;
    bool must_update = true;
    float loading_state_parse = 0.0f;
    float loading_state_compute[3] = {0.0f, 0.0f, 0.0f};
    bool exit = false;
    bool failed_to_load = false;
    std::string file_load_error;
    float furthest_point_center_distance;
    float furthest_point_zero_distance;
    // average of all points
    vec3<float> center_average;
    // center of the bounding box of the points
    vec3<float> center_bounding;
    vec3<float> bounding_box_low;
    vec3<float> bounding_box_high;
    void SetLoadError(std::string text)
    {
        Lock();
        failed_to_load = true;
        file_load_error = text;
        Unlock();
    }
    bool LoadFile(std::string path)
    {
        FILE* file = fopen(path.c_str(), "r");
        if(file == nullptr)
        {
            SetLoadError("Failed to open file!");
            return false;
        }
        file_size = std::filesystem::file_size(path);
        while(true)
        {
            float x, y, z;
            int res = fscanf(file, "%f%f%f", &x, &y, &z);
            
            if(res == EOF)
                break;
            if(res != 3)
            {
                SetLoadError("Failed to parse file, invalid format");
                fclose(file);
                return false;
            }
            if(isnan(x) || isnan(y) || isnan(z))
            {    
                SetLoadError("Failed to parse file: invalid value(s) encountered");
                fclose(file);
                return false;
            }
            loading_state_parse += float(ASSUMED_BYTES_PER_LINE)/float(file_size);
            points.push_back({x, y, z});
        }
        fclose(file);
        if(points.size() == 0)
        {
            SetLoadError("No data found in file");
            return false;
        }
        bounding_box_high = points[0];
        bounding_box_low = points[0];
        // find center and the furthest point from (0, 0, 0)
        // also find the bounding box
        furthest_point_zero_distance = 0.0f;
        center_average = vec3<float>{0, 0, 0};
        for(int i = 0; i < points.size(); i++)
        {
            auto& p = points[i];
            center_average += p/float(points.size());
            float l = p.length();
            if(l > furthest_point_zero_distance)
                furthest_point_zero_distance =  l;
            
            bounding_box_high = p.max(bounding_box_high);
            bounding_box_low = p.min(bounding_box_low);

            loading_state_compute[0] = float(i)/float(points.size());
        }
        center_bounding = (bounding_box_low + bounding_box_high) / 2.0f;

        // find greatest distance from center
        vec3<float> furthest_point_center;
        furthest_point_center_distance = 0.0f;
        for(int i = 0; i < points.size(); i++)
        {
            auto p = points[i] - center_average;
            float l = p.length();
            if(l > furthest_point_center_distance)
            {
                furthest_point_center_distance = l;
                furthest_point_center = p;
            }
            loading_state_compute[1] = float(i)/float(points.size());
        }

        // sort by the Z axis, ascending
        // also approximates, which probably slows this down quite a bit
        std::sort(points.begin(), points.end(), [&](vec3<float> l, vec3<float> r) {loading_state_compute[2] += 0.1f * 1.0f/float(points.size()); return l.z < r.z;});
        loading_state_compute[2] = 1.0f;
        memory_used = sizeof(points[0]) * points.size();
        return true;
    }
    void ProcessingFunction()
    {
        if(!LoadFile(path))
            return;      
        while(true)
        {
            if(!must_update)
            {
                auto lock = std::unique_lock<std::mutex>(notify_mx);
                process_notify.wait(lock);
            }
            
            if(exit)
                return;
            Lock();
            std::vector<float> sections = this->sections;
            must_update = false;
            Unlock();
            if(sections.size() != 0)
            {
                int current_section = 0;
                std::vector<size_t> section_indices;
                section_indices.resize(sections.size(), 0);
                float pos = sections[0];
                for(size_t i = 0; i < points.size(); i++)
                {
                    auto p = points[i];
                    if(p.z > pos)
                    {
                        section_indices[current_section] = i;
                        current_section++;
                        if(current_section == sections.size())
                            break;
                        pos += sections[current_section];
                    }
                }
                if(current_section != sections.size())
                    section_indices[current_section] = points.size();
                this->section_indices = section_indices;
            }
            Lock();
            is_loaded = true;
            Unlock();
        }
    }
    void ProcessorNotify()
    {
        auto lock = std::unique_lock<std::mutex>(notify_mx);
        lock.unlock();
        process_notify.notify_one();
    }
    public:
    // Lock() required
    bool IsLoaded()
    {
        return is_loaded;
    }
    // Lock() required
    bool HasFailedToLoad()
    {
        return failed_to_load;
    }
    // Not guaranteed to return a valid value, thread safety would be too expensive and glitches are unlikely and acceptable
    // Does not indicate whether the load has been completed
    float LoadingState()
    {
        float v = 0.0f;
        v += std::min(1.0f, loading_state_parse) * WEIGHT_PARSE;
        for(int i = 0; i < ArraySize(loading_state_compute); i++)
        {
            v += std::min(1.0f, loading_state_compute[i]) * WEIGHT_COMPUTE;
        }
        return v;
    }
    std::string GetLoadFailureError()
    {
        assert(HasFailedToLoad());
        return file_load_error;
    }
    std::string GetFileName()
    {
        return file_name;
    }
    std::string GetFilePath()
    {
        return path;
    }
    size_t GetFileSize()
    {
        return file_size;
    }
    size_t GetMemoryUsage()
    {
        return memory_used;
    }
    size_t GetNPoints()
    {
        return points.size();
    }
    float GetFurthestDistanceFromZero()
    {
        return furthest_point_zero_distance;
    }
    float GetFurthestDistanceFromCenter()
    {
        return furthest_point_center_distance;
    }
    vec3<float> GetCenterAverage()
    {
        return center_average;
    }
    vec3<float> GetCenterBounding()
    {
        return center_bounding;
    }
    vec3<float> GetBoundingBoxLow()
    {
        return bounding_box_low;
    }
    vec3<float> GetBoundingBoxHigh()
    {
        return bounding_box_high;
    }
    // Lock() required
    void SetSections(std::vector<float>& sections)
    {
        assert(IsLoaded());
        assert(sections.size() > 0);
        this->sections = sections;
        must_update = true;
        ProcessorNotify();
    }
    void GetPointsSorted(std::vector<vec3<float>>* out)
    {
        assert(IsLoaded());
        *out = points;
    }
    // Lock() required
    std::vector<size_t> GetSectionIndices()
    {
        assert(IsLoaded());
        return section_indices;
    }
    void Lock()
    {
        access_mx.lock();
    }
    void Unlock()
    {
        access_mx.unlock();
    }
    bool TryLock()
    {
        return access_mx.try_lock();
    }
    PointProcessor(std::string path)
    {
        assert(std::filesystem::exists(path));
        this->path = std::filesystem::absolute(path);
        this->file_name = std::filesystem::path(path).filename();
        processing_thread = std::thread(&PointProcessor::ProcessingFunction, this);
    }
    ~PointProcessor()
    {
        exit = true;
        if(!failed_to_load)
            ProcessorNotify();
        processing_thread.join();
    }
};