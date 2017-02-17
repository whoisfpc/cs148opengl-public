#include "assignment2/Assignment2.h"
#include "common/core.h" // <-- haha.
#include "common/Utility/Mesh/Simple/PrimitiveCreator.h"
#include "common/Utility/Mesh/Loading/MeshLoader.h"
#include <cmath>
#include <fstream>

namespace
{
const int SHADER_ERROR_LOG_SIZE = 500;
bool VerifyShaderCompile(GLuint newShaderObject)
{
    GLint compileStatus;
    OGL_CALL(glGetShaderiv(newShaderObject, GL_COMPILE_STATUS, &compileStatus));
    if (compileStatus != GL_TRUE) {
        char errorLogBuffer[SHADER_ERROR_LOG_SIZE];
        OGL_CALL(glGetShaderInfoLog(newShaderObject, SHADER_ERROR_LOG_SIZE, NULL, errorLogBuffer));
        std::cerr << "ERROR: Shader compilation failure -- " << std::endl << errorLogBuffer << std::endl;
        return false;
    }
    return true;
}

bool VerifyProgramLink(GLuint shaderProgram)
{
    GLint linkStatus;
    OGL_CALL(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus));
    if (linkStatus != GL_TRUE) {
        char errorLogBuffer[SHADER_ERROR_LOG_SIZE];
        OGL_CALL(glGetProgramInfoLog(shaderProgram, SHADER_ERROR_LOG_SIZE, NULL, errorLogBuffer));
        std::cerr << "ERROR: Program link compilation failure -- " << std::endl << errorLogBuffer << std::endl;
        return false;
    }
    return true;
}
}

Assignment2::Assignment2(std::shared_ptr<class Scene> inputScene, std::shared_ptr<class Camera> inputCamera):
    Application(std::move(inputScene), std::move(inputCamera))
{
    vertexPositions = 
        std::initializer_list<glm::vec4>({
            // Triangle 1
            {0.f, 0.f, 0.f, 1.f},
            {1.f, 0.f, 0.f, 1.f},
            {1.f, 1.f, 0.f, 1.f},
            // Triangle 2
            {0.f, 0.f, 0.f, 1.f},
            {-1.f, 1.f, 0.f, 1.f},
            {-1.f, 0.f, 0.f, 1.f}
        });

	time = 0.0f;
}

std::unique_ptr<Application> Assignment2::CreateApplication(std::shared_ptr<class Scene> scene, std::shared_ptr<class Camera> camera)
{
    return make_unique<Assignment2>(std::move(scene), std::move(camera));
}

std::shared_ptr<class Camera> Assignment2::CreateCamera()
{
    // Specify any old aspect ratio for now, we'll update it later once the window gets made!
    // Read more about Field of View: http://rg3.name/201210281829.html!
    // Note that our field of view is the VERTICAL field of view (in degrees).
    return std::make_shared<PerspectiveCamera>(75.f, 1280.f / 720.f);
}

glm::vec2 Assignment2::GetWindowSize() const
{
    return glm::vec2(1280.f, 720.f);
}

void Assignment2::SetupScene()
{
    SetupExample1();
}

void Assignment2::SetupCamera()
{
}

void Assignment2::HandleInput(SDL_Keysym key, Uint32 state, Uint8 repeat, double timestamp, double deltaTime)
{   
    Application::HandleInput(key, state, repeat, timestamp, deltaTime);
}

void Assignment2::HandleWindowResize(float x, float y)
{
    Application::HandleWindowResize(x, y);
}

void Assignment2::SetupExample1()
{
    // Insert "Load and Compile Shaders" code here.
	const std::string vertPath = std::string(STRINGIFY(SHADER_PATH)) + "/hw2/hw2.vert";
	const std::string fragPath = std::string(STRINGIFY(SHADER_PATH)) + "/hw2/hw2.frag";
	std::ifstream vertIn(vertPath);
	std::ifstream fragIn(fragPath);
	std::string vertStr((std::istreambuf_iterator<char>(vertIn)), std::istreambuf_iterator<char>());
	std::string fragStr((std::istreambuf_iterator<char>(fragIn)), std::istreambuf_iterator<char>());

	const GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *c_vertStr = vertStr.c_str();
	const GLchar *c_fragStr = fragStr.c_str();
	glShaderSource(vertShader, 1, &c_vertStr, NULL);
	glShaderSource(fragShader, 1, &c_fragStr, NULL);
	glCompileShader(vertShader);
	glCompileShader(fragShader);
	program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);
    // Checkpoint 1.
    // Modify this part to contain your vertex shader ID, fragment shader ID, and shader program ID.
	const GLuint vertexShaderId = vertShader;
	const GLuint fragmentShaderId = fragShader;
	const GLuint shaderProgramId = program;

    // DO NOT EDIT OR REMOVE THE CODE IN THIS SECTION
    if (!VerifyShaderCompile(vertexShaderId) || !VerifyShaderCompile(fragmentShaderId) || !VerifyProgramLink(shaderProgramId)) {
        std::cout << "FAILURE: Checkpoint 1 failed." << std::endl;
    } else {
        std::cout << "SUCCESS: Checkpoint 1 completed." << std::endl;
    }

    OGL_CALL(glDetachShader(shaderProgramId, vertexShaderId));
    OGL_CALL(glDeleteShader(vertexShaderId));
    OGL_CALL(glDetachShader(shaderProgramId, fragmentShaderId));
    OGL_CALL(glDeleteShader(fragmentShaderId));
    // FINISH DO NOT EDIT OR REMOVE THE CODE IN THIS SECTION

    // Insert "Setup Buffers" code here.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLuint bufferId;
	glGenBuffers(1, &bufferId);
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexPositions.size(), &vertexPositions[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

void Assignment2::Tick(double deltaTime)
{
    // Insert "Send Buffers to the GPU" and "Slightly-More Advanced Shaders" code here.
	time += deltaTime;
	glUseProgram(program);
	GLint timeLocation = glGetUniformLocation(program, "inputTime");
	glUniform1f(timeLocation, time);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertexPositions.size());

}
