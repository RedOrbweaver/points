#pragma once

static constexpr glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
static constexpr glm::vec3 target = glm::vec3(0.f, 0.f, 0.f);

class OrbitCamera
{
private:


	float distance = 100;
	float theta = 0;
	float phi = 0;

    float fov;
    float near_plane;
    float far_plane;
    float aspect_ratio;

    glm::vec3 center;
public:

    float GetDistance()
    {
        return distance;
    }
    float GetPhi()
    {
        return phi;
    }
    float GetTheta()
    {
        return theta;
    }

    void SetDistance(float value)
    {
        distance = value;
    }
    void GetPhi(float value)
    {
        phi = value;
    }
    void GetTheta(float value)
    {
        theta = value;
    }
    void SetFOV(float value)
    {
        fov = value;
    }
    void SetNearPlane(float value)
    {
        near_plane = value;
    }
    void SetFarPlane(float value)
    {
        far_plane = value;
    }
    void SetAspectRatio(float value)
    {
        aspect_ratio = value;
    }

	void AddDistance(float deltaRadius)
    {
        distance += deltaRadius;
        if(distance < 1.0f)
            distance = 1.0f;
    }

    void SetCenter(vec3<float> center)
    {
        this->center.x = center.x;
        this->center.y = center.y;
        this->center.z = center.z;
    }

	void Rotate(float theta, float phi)
    {
        this->theta += theta;
        this->phi += phi;
        if(this->phi > M_PI_4)
            this->phi = M_PI_4;
        else if(this->phi < -M_PI_4)
            this->phi = -M_PI_4;
    }   

	glm::mat4 GetModelViewMatrix()
    {
        glm::vec3 eye
        (
            (distance * -glm::sin(theta) * glm::cos(phi)),
            (distance * -glm::sin(phi)),
            (distance * glm::cos(theta) * glm::cos(phi))
        );
        return glm::lookAt(eye, center, glm::vec3{0.0f, 1.0f, 0.0f});
    }
    glm::mat4 GetProjectionMatrix()
    {
        return glm::perspective(fov, aspect_ratio, near_plane, far_plane);
    }

    OrbitCamera(float fov, float near_plane, float far_plane, float aspect_ratio)
    {
        center = glm::vec3(0.0f, 0.0f, 0.0f);
        this->fov = fov;
        this->near_plane = near_plane;
        this->far_plane = far_plane;
        this->aspect_ratio = aspect_ratio;
    }

};