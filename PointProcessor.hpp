#pragma once

#include "hmain.hpp"

class PointProcessor
{
    protected:
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
    bool exit = false;
    bool failed_to_load = false;
    std::string file_load_error;
    float furthest_point_center;
    float furthest_point_zero;
    vec3<float> center;
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
            points.push_back({x, y, z});
        }
        fclose(file);
        if(points.size() == 0)
        {
            SetLoadError("No data found in file");
            return false;
        }
        file_size = std::filesystem::file_size(path);
        // find center and the furthest point from (0, 0, 0)
        furthest_point_zero = 0.0f;
        center = vec3<float>{0, 0, 0};
        for(int i = 0; i < points.size(); i++)
        {
            auto& p = points[i];
            center += p/float(points.size());
            float l = p.length();
            if(l > furthest_point_zero)
                furthest_point_zero =  l;
        }
        // find greatest distance from center
        furthest_point_center = 0.0f;
        for(int i = 0; i < points.size(); i++)
        {
            auto p = points[i] - center;
            float l = p.length();
            if(l > furthest_point_center)
                furthest_point_center = l;
        }
        // sort by the Z axis, ascending
        std::sort(points.begin(), points.end(), [](vec3<float> l, vec3<float> r) {return l.z < r.z;});
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
                        pos += sections[current_section];
                        current_section++;
                        if(current_section == sections.size())
                            break;
                    }
                }
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
        return furthest_point_zero;
    }
    float GetFurthestDistanceFromCenter()
    {
        return furthest_point_center;
    }
    vec3<float> GetCenter()
    {
        return center;
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