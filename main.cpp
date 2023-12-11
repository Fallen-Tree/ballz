#include <math.h>
#include<iostream>

#include "engine.hpp"

static int SCR_HEIGHT = 600;
static int SCR_WIDTH = 800;

const char *vertexShaderSource = "/vertex/standart.vshader";
const char *fragmentShaderSource = "/fragment/standart.fshader";

const char *cubeSource = "/cube.obj";

class MovingBall : public Object {
 private:
    float MIN_VELOCITY = 0.01;
 public:
    Vec3 velocity;
    float bounciness = 1;
    float friction = 2;

    void Update(float dt) override {
        transform->Translate(velocity * dt);
        // transform->Rotate(length(velocity) * dt / transform->GetScale().x / 2, direction);
        if (length(velocity) < MIN_VELOCITY) {
            velocity = Vec3(0, 0, 0);
        } else {
            // Can be dangerous
            velocity -= glm::normalize(velocity) * friction * dt;
        }
    }

    MovingBall(Vec3 position, ShaderProgram * shaderProgram,
     std::string diffuseSource, std::string specularSource = "") {
        this->renderData = new RenderData();
        this->renderData->model = Model::GetSphere();
        this->renderData->model->shader = shaderProgram;

        auto images = std::vector<std::string>();
        images.push_back(diffuseSource);
        if (specularSource != "") {
            images.push_back(specularSource);
        }
        this->renderData->material = {
            4.f,
            Texture(images),
        };

        bindRenderData(this->renderData);

        this->collider = new Collider{Sphere{
            Vec3(0.0),
            1.0f,
        }};

        transform = new Transform(position, Vec3(1., 1., 1.), Mat4(1.0));
    }
};

class GameManager : public Object {
 public:
    std::vector<MovingBall*> balls;
    float minX = -7, maxX = 7, minZ = -7, maxZ = 7;

    void Update(float dt) override {
        for (int i = 0; i < balls.size(); ++i) {
            auto ball = balls[i];
            Vec3 pos = ball->transform->GetTranslation();
            if (pos.x < minX) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(minX - pos.x, 0, 0));
                ball->velocity = Vec3(- ball->velocity.x, 0, ball->velocity.z) * ball->bounciness;
            }
            if (pos.x > maxX) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(maxX - pos.x, 0, 0));
                ball->velocity = Vec3(- ball->velocity.x, 0, ball->velocity.z) * ball->bounciness;
            }
            if (pos.z < minZ) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(0, 0, minZ - pos.z));
                ball->velocity = Vec3(ball->velocity.x, 0, - ball->velocity.z) * ball->bounciness;
            }
            if (pos.z > maxZ) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(0, 0, maxZ - pos.z));
                ball->velocity = Vec3(ball->velocity.x, 0, - ball->velocity.z) * ball->bounciness;
            }
            for (int j = i + 1; j < balls.size(); ++j) {
                auto ball2 = balls[j];
                if (ball->collider->Collide(*ball->transform, ball2->collider, *ball2->transform)) {
                    // collision
                    Vec3 pos2 = ball2->transform->GetTranslation();
                    Vec3 center = (pos + pos2) * 0.5f;  // collision point
                    Vec3 speed = ball->velocity - ball2->velocity;  // approach speed of 2 balls
                    Vec3 v = pos - center;  // vector towards center of collision
                    float dotProduct = speed.x * v.x + speed.z * v.z;
                    float proj = dotProduct / length(v);  // projection of speed vector on collision line
                    Vec3 impulseChange = normalize(v) * proj * (1 + ball->bounciness * ball2->bounciness);
                    ball->velocity -= impulseChange * 0.5f;
                    ball2->velocity += impulseChange * 0.5f;
                    ball->transform->Translate(v * (ball->transform->GetScale().x / length(v) - 1));
                    ball2->transform->Translate(v * (1 - ball2->transform->GetScale().x / length(v)));
                }
            }
        }
    }
    explicit GameManager(std::vector<MovingBall*> balls) {
        this->balls = balls;
    }
};

MovingBall *newBall(Vec3 position, Vec3 velocity,
        ShaderProgram *sp, std::string diffuseSource, std::string specularSource);

int main() {
    auto engine = Engine(SCR_WIDTH, SCR_HEIGHT);

    Shader vShader = Shader(VertexShader, vertexShaderSource);
    Shader fShader = Shader(FragmentShader, fragmentShaderSource);
    ShaderProgram *shaderProgram = new ShaderProgram(vShader, fShader);

    std::vector<MovingBall*> balls;
    int ballsCount = 5;

    for (int i = 0; i < ballsCount; ++i) {
        MovingBall *sphere = newBall(Vec3(i * 2 - 5, -12, -i), Vec3(5, 0, 2 + 2 * i),
         shaderProgram, "/wall.png", "/wallspecular.png");
        balls.push_back(sphere);
        engine.AddObject<>(sphere);
    }

    GameManager *gameManager = new GameManager(balls);

    engine.AddObject(gameManager);

    engine.Run(SCR_WIDTH, SCR_HEIGHT);
}

MovingBall *newBall(Vec3 position, Vec3 velocity,
        ShaderProgram *sp, std::string diffuseSource, std::string specularSource) {
    MovingBall* ball = new MovingBall(position, sp, diffuseSource, specularSource);
    ball->velocity = velocity;
    return ball;
}
