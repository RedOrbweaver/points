#pragma once

static constexpr glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
static constexpr glm::vec3 target = glm::vec3(0.f, 0.f, 0.f);

class OrbitCamera
{
private:


	GLfloat distance;
	GLfloat theta;
	GLfloat phi;

    GLfloat fov;
    GLfloat near_plane;
    GLfloat far_plane;
    GLfloat aspect_ratio;
public:

    GLfloat GetDistance()
    {
        return distance;
    }
    GLfloat GetPhi()
    {
        return phi;
    }
    GLfloat GetTheta()
    {
        return theta;
    }

    void SetDistance(GLfloat value)
    {
        distance = value;
    }
    void GetPhi(GLfloat value)
    {
        phi = value;
    }
    void GetTheta(GLfloat value)
    {
        theta = value;
    }
    void SetFOV(GLfloat value)
    {
        fov = value;
    }
    void SetNearPlane(GLfloat value)
    {
        near_plane = value;
    }
    void SetFarPlane(GLfloat value)
    {
        far_plane = value;
    }
    void SetAspectRatio(GLfloat value)
    {
        aspect_ratio = value;
    }

	void AddDistance(GLfloat deltaRadius)
    {
        distance += deltaRadius;
        assert(distance > 0);
    }

	void Rotate(GLfloat theta, GLfloat phi)
    {
        this->theta += theta;
        this->phi += phi;
    }   

	glm::mat4 GetModelViewMatrix()
    {
        glm::vec3 eye
        (
            (distance * -glm::sin(theta) * glm::cos(phi)),
            (distance * -glm::sin(phi)),
            (distance * glm::cos(theta) * glm::cos(phi))
        );
        return glm::lookAt(eye, glm::vec3{0, 0, 0}, glm::vec3{0.f, 1.f, 0.f});
    }
    glm::mat4 GetProjectionMatrix()
    {
        return glm::perspective(M_PI/4.0, 16.0/9.0, 0.0, 1000.0);;
    }

    OrbitCamera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane, GLfloat aspectRatio)
    {
        
    }

};