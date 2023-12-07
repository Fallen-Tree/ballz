#include <math.h>

#include "engine.hpp"

static int SCR_HEIGHT = 600;
static int SCR_WIDTH = 800;

const char *vertexShaderSource = "/vertex/standart.vshader";
const char *fragmentShaderSource = "/fragment/standart.fshader";

const char *cubeSource = "/cube.obj";

class MovingSphere : public Object {
 private:
    float MIN_VELOCITY = 0.001;
 public:
    Vec3 velocity;
    float bounciness = 1;
    float friction = 0.1;

    void Update(float dt) override {
        transform->Translate(velocity * dt);
        velocity *= pow(friction, dt);
        if (length(velocity) < MIN_VELOCITY) {
            velocity = Vec3(0, 0, 0);
        }
    }
};

int main() {
    auto engine = Engine(SCR_WIDTH, SCR_HEIGHT);

    Shader vShader = Shader(VertexShader, vertexShaderSource);
    Shader fShader = Shader(FragmentShader, fragmentShaderSource);
    ShaderProgram *shaderProgram = new ShaderProgram(vShader, fShader);

    Model *cubeModel = Model::loadFromFile(cubeSource);
    cubeModel->shader = shaderProgram;
    Model *sphereModel = Model::GetSphere();
    sphereModel->shader = shaderProgram;

    auto imagesCube = std::vector<std::string>();
    imagesCube.push_back("/wall.png");
    imagesCube.push_back("/wallspecular.png");
    Material material = {
        4.f,
        Texture(imagesCube),
    };

    auto setUpObj = [=, &engine](Transform transform, auto primitive, Model *model) {
        auto obj = new Object();
        obj->renderData = new RenderData();
        auto renderData = obj->renderData;
        renderData->model = model;
        renderData->material = material;
        bindRenderData(renderData);

        obj->transform = new Transform(transform);
        obj->collider = new Collider { primitive };
        engine.AddObject<>(obj);
        return obj;
    };

    auto aabb = setUpObj(
        Transform(Vec3(-4, 0, -3), Vec3(1.0), 0, Vec3(1)),
        AABB {
            Vec3{-0.5, -0.5, -0.5},
            Vec3{0.5, 0.5, 0.5},
        },
        cubeModel);

    auto sphere = setUpObj(
        Transform(Vec3(0, 0, -3), Vec3(1.0), 0, Vec3(1)),
        Sphere { Vec3(0.0), 1.0f },
        sphereModel);

    auto mesh = setUpObj(
        Transform(Vec3(4.0, 0, -3), Vec3(1.0), 0, Vec3(1)),
        sphereModel,
        sphereModel);

    auto getSphereObj = [=](Transform transform, Object *target, Vec3 speed) {
        auto obj = new MovingSphere();
        obj->renderData = new RenderData();
        auto renderData = obj->renderData;
        renderData->model = sphereModel;
        renderData->material = material;
        bindRenderData(renderData);

        obj->transform = new Transform(transform);

        obj->collider = new Collider{Sphere{
            Vec3(0.0),
            1.0f,
        }};
        return obj;
    };

    Object *spheres[3] = {
        getSphereObj(
            Transform(Vec3(-4, 0, 2.0), Vec3(1.0), 0, Vec3(1)),
            aabb,
            Vec3(0, 0, -1)),
        getSphereObj(
            Transform(Vec3(0, 0, 2.0), Vec3(1.0), 0, Vec3(1)),
            sphere,
            Vec3(0, 0, -1)),
        getSphereObj(
            Transform(Vec3(4, 0, 2.0), Vec3(1.0), 0, Vec3(1)),
            mesh,
            Vec3(0, 0, -1)),
    };
    engine.AddObject<>(spheres[0]);
    engine.AddObject<>(spheres[1]);
    engine.AddObject<>(spheres[2]);
    
    engine.Run(SCR_WIDTH, SCR_HEIGHT);
}
