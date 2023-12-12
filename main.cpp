#include<math.h>
#include<iostream>

#include "logger.hpp"
#include "engine.hpp"

const char *vertexShaderSource = "/vertex/standart.vshader";
const char *fragmentShaderSource = "/fragment/standart.fshader";

const char *cubeSource = "/cube.obj";

class MovingBall : public Object {
 private:
    float MIN_VELOCITY = 0.001;

 public:
    bool active = true;
    Vec3 velocity;
    float bounciness = 1;
    float friction = 4;

    void Update(float dt) override {
        transform->Translate(velocity * dt);
        Vec3 rotationAxis = cross(normalize(velocity), Vec3(0, -1, 0));
        float angle = length(velocity) * dt / transform->GetScale().x;
        if (length(velocity) < MIN_VELOCITY) {
            velocity = Vec3(0, 0, 0);
        } else {
            Mat4 rotation = transform->GetRotation();
            transform->Rotate(inverse(rotation));
            transform->Rotate(angle, rotationAxis);
            transform->Rotate(rotation);
            // Can be dangerous
            velocity -= glm::normalize(velocity) * friction * dt;
        }
    }

    MovingBall(Vec3 position, float radius, ShaderProgram * shaderProgram,
     std::string diffuseSource, std::string specularSource = "") {
        this->renderData = new RenderData();
        this->renderData->model = Model::loadFromFile("/shar_152.obj");  // Model::GetSphere();
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
            radius,
        }};

        transform = new Transform(position, Vec3(radius), Mat4(1.0));
    }
};

class Hole : public Object {
 private:
    Vec3 NOWHERE = Vec3(0, 10000, 0);
 public:
    Hole(Vec3 position, float r) {
        this->collider = new Collider{Sphere{
            Vec3(0.0),
            r,
        }};

        transform = new Transform(position, Vec3(r), Mat4(1.0));
    }

    void Consume(MovingBall *ball) {
        ball->active = false;
        ball->velocity = Vec3(0, 0, 0);
        ball->transform->SetTranslation(transform->GetTranslation());
        ball->animation = new Animation();
        auto newTrans = *ball->transform;
        newTrans.Translate(Vec3(0, -2, 0));
        ball->animation->addAnimation(newTrans, 2.f);
    }
};

class Cue : public Object {
 public:
     Cue(std::vector<MovingBall *> objects, Camera *camera, ShaderProgram *shader) {
        m_Objects = objects;
        m_Camera = camera;
        m_CueDistance = 3.f;
        renderData = new RenderData();
        renderData->model = Model::loadFromFile("/cue.obj");
        renderData->model->shader = shader;
        bindRenderData(renderData);

        transform = new Transform(Vec3(0), Vec3(8), 0, Vec3(1));

        auto imagesCue = std::vector<std::string>();
        imagesCue.push_back("/kiy.png");
        imagesCue.push_back("/kiy.png");
        renderData->material = {
            4.f,
            Texture(imagesCue),
        };
     }

    void Update(float dt) override {
        Ray ray = Ray(m_Camera->GetPosition(), m_Camera->GetPosition() + m_Camera->GetFront());
        // m_Camera->GetRayThroughScreenPoint({s_Input->MouseX(), s_Input->MouseY()});;
        MovingBall *target = nullptr;
        float closest = std::numeric_limits<float>::max();
        for (int i = 0; i < m_Objects.size(); i++) {
            auto obj = m_Objects[i];
            auto hit = obj->collider->RaycastHit(*obj->transform, ray);
            if (hit && *hit < closest) {
                target = obj;
                closest = *hit;
            }
        }
        if (s_Input->IsKeyPressed(Key::MouseLeft)) {
            if (m_CurrentTarget != nullptr) {
                Vec3 direction = m_CurrentTarget->transform->GetTranslation() - transform->GetTranslation();
                direction.y = 0;
                m_CurrentTarget->velocity += 6.f * direction;
                m_CurrentTarget = nullptr;
            }

            if (m_CurrentTarget == nullptr && target != nullptr)
                m_CurrentTarget = target;
        }

        if (s_Input->IsKeyPressed(Key::MouseRight))
            m_CurrentTarget = nullptr;

        if (m_CurrentTarget != nullptr) {
            Vec3 center = m_CurrentTarget->transform->GetTranslation();
            Vec3 closest = ray.origin + glm::dot(center - ray.origin, ray.direction) * ray.direction;
            closest.y = center.y;
            Vec3 onCircle = center + glm::normalize(closest - center) * m_CueDistance;
            transform->SetTranslation(onCircle);

            Vec3 toCenter = center - onCircle;
            float angle = glm::acos(glm::dot(toCenter, ray.direction) / m_CueDistance);
            transform->SetRotation(-glm::pi<float>()/2, glm::cross(Vec3{0.f, 1.f, 0.f}, toCenter));
        } else {
            transform->SetRotation(glm::pi<float>(), Vec3(1.f, 0.f, 0.f));
            transform->SetTranslation(m_Camera->GetPosition() + Vec3{0.5f, -0.5f, -0.5f});
        }
     }

 private:
    float m_CueDistance = 1.f;
    MovingBall *m_CurrentTarget = nullptr;
    std::vector<MovingBall *> m_Objects;
    Camera *m_Camera;
};


class GameManager : public Object {
 private:
    int score = 0;
    void ShowScore() {
        char buf[15];
        snprintf(buf, sizeof(buf), "Score: %d", score);
        this->text->SetContent(buf);
    }

 public:
    std::vector<MovingBall*> balls;
    std::vector<Hole*> holes;

    float minX = -10, maxX = 10, minZ = -10, maxZ = 10;

    void SetBorders(float minX, float maxX, float minZ, float maxZ) {
        this->minX = minX;
        this->maxX = maxX;
        this->minZ = minZ;
        this->maxZ = maxZ;
    }

    void Update(float dt) override {
        for (int i = 0; i < balls.size(); ++i) {
            auto ball = balls[i];
            if (!ball->active) continue;
            Vec3 pos = ball->transform->GetTranslation();
            float r = ball->transform->GetScale().x;
            for (auto h : holes) {
                if (ball->collider->Collide(*ball->transform, h->collider, *h->transform)) {
                    h->Consume(ball);
                    score += 100;
                    break;
                }
            }
            if (!ball->active) continue;
            if (pos.x - r < minX) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(minX - pos.x + r, 0, 0));
                ball->velocity = Vec3(- ball->velocity.x, 0, ball->velocity.z) * ball->bounciness;
            }
            if (pos.x + r > maxX) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(maxX - pos.x - r, 0, 0));
                ball->velocity = Vec3(- ball->velocity.x, 0, ball->velocity.z) * ball->bounciness;
            }
            if (pos.z - r < minZ) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(0, 0, minZ - pos.z + r));
                ball->velocity = Vec3(ball->velocity.x, 0, - ball->velocity.z) * ball->bounciness;
            }
            if (pos.z + r > maxZ) {
                ball->transform->Translate((1 + ball->bounciness) * Vec3(0, 0, maxZ - pos.z - r));
                ball->velocity = Vec3(ball->velocity.x, 0, - ball->velocity.z) * ball->bounciness;
            }
            for (int j = i + 1; j < balls.size(); ++j) {
                auto ball2 = balls[j];
                if (!ball2->active) continue;
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
                    ball->transform->Translate(v * (r / length(v) - 1));
                    ball2->transform->Translate(v * (1 - r / length(v)));  // r2 == r
                }
            }
        }
        ShowScore();
    }

    explicit GameManager(std::vector<MovingBall*> balls, std::vector<Hole*> holes) {
        this->balls = balls;
        this->holes = holes;
    }
};

class Table : public Object {
 public:
    explicit Table(Vec3 position, Vec3 scale, ShaderProgram *shaderProgram) {
        this->renderData = new RenderData();
        this->renderData->model = Model::loadFromFile("/stol_1.obj");
        this->renderData->model->shader = shaderProgram;

        auto images = std::vector<std::string>();
        images.push_back("/stol.png");
        this->renderData->material = {
            4.f,
            Texture(images),
        };

        bindRenderData(this->renderData);

        transform = new Transform(position, scale, Mat4(1.0));
    }
};

class FpsText : public Object {
 public:
    void Update(float dt) override {
        int fps = Time::GetCurrentFps();
        char buf[12];
        snprintf(buf, sizeof(buf), "Fps: %d", fps);
        this->text->SetContent(buf);
    }
};

MovingBall *newBall(Vec3 position, Vec3 velocity, float radius,
        ShaderProgram *sp, std::string diffuseSource, std::string specularSource);

void initLight(Engine& engine);

int main() {
    auto engine = Engine();

    Shader vShader = Shader(VertexShader, vertexShaderSource);
    Shader fShader = Shader(FragmentShader, fragmentShaderSource);
    ShaderProgram *shaderProgram = new ShaderProgram(vShader, fShader);

    initLight(engine);

    std::vector<MovingBall*> balls;
    int ballsCount = 5;
    float ballRadius = 0.3f;
    float Y = -8;

    for (int i = 0; i < ballsCount; ++i) {
        MovingBall *sphere = newBall(Vec3(i * 2 - 5, Y, -i), Vec3(3, 0, 1 + i), ballRadius,
         shaderProgram, "/152.png", "/Cat_specular.png");
        balls.push_back(sphere);
        engine.AddObject<>(sphere);
    }

    std::vector<Hole*> holes;
    float holeRadius = 0.2f;
    float borderX = 7.17f;
    float borderZ = 3.59f;
    float holeX = borderX - 0.25f;
    float holeZ = borderZ - 0.25f;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            Hole *newHole = new Hole(Vec3(-holeX + 2 * j * holeX, Y, -holeZ + i * 2 * holeZ), holeRadius);
            holes.push_back(newHole);
        }
        Hole *newHole = new Hole(Vec3(0, Y, -borderZ + i * 2 * borderZ), holeRadius);
        holes.push_back(newHole);
    }

    GameManager *gameManager = new GameManager(balls, holes);
    gameManager->SetBorders(-borderX, borderX, -borderZ, borderZ);

    Cue *cue = new Cue(balls, engine.camera, shaderProgram);
    engine.AddObject(cue);

    /* Fps counter */
    auto textOcra = new Font("OCRAEXT.TTF", 20);
    auto fpsObj = new FpsText();
    fpsObj->text = new Text(textOcra, "", 685.0f, 575.0f, 1.f, Vec3(0, 0, 0));
    engine.AddObject<>(fpsObj);;

    /* Pointer */
    auto pointer = new Object();
    auto textOcraBig = new Font("OCRAEXT.TTF", 40);
    pointer->text = new Text(textOcraBig, "+", SCR_WIDTH/2.f - 10, SCR_HEIGHT/2.f - 10, 1.f, Vec3(0, 0, 0));
    engine.AddObject<>(pointer);

    gameManager->text = new Text(textOcra, "", 10.0f, 575.f, 1.f, Vec3(0, 0, 0));
    engine.AddObject(gameManager);

    Vec3 tablePosition = Vec3(0, -15.04, 0);
    Vec3 tableScale = Vec3(8, 8, 8);
    Table *table = new Table(tablePosition, tableScale, shaderProgram);

    engine.AddObject(table);

    engine.Run();
}

MovingBall *newBall(Vec3 position, Vec3 velocity, float radius,
        ShaderProgram *sp, std::string diffuseSource, std::string specularSource) {
    MovingBall* ball = new MovingBall(position, radius, sp, diffuseSource, specularSource);
    ball->velocity = velocity;
    return ball;
}

void initLight(Engine& engine) {
    // init light objects
    Object* pointLight1 = new Object();
    pointLight1->light = new PointLight(
        Vec3(0.2f, 0.2f, 0.2f), Vec3(0.8f, 0.8f, 0.8f),
        Vec3(1.0f, 1.0f, 1.0f), Vec3(-0.2, -0.5, -1.2),
        1, 0.09f, 0.032f);
    auto pointLight = std::get<PointLight*>(pointLight1->light);
    engine.AddObject<>(pointLight1);

    Object* pointLight2 = new Object();
    pointLight2->light = new PointLight(
        Vec3(0.2f, 0.2f, 0.2f), Vec3(0.8f, 0.8f, 0.8f),
        Vec3(1.0f, 1.0f, 1.0f), Vec3(2.3f, -3.3f, -4.0f),
        1, 0.09f, 0.032f);
    pointLight = std::get<PointLight*>(pointLight2->light);
    engine.AddObject<>(pointLight2);

    Object* pointLight3 = new Object();
    pointLight3->light = new PointLight(
        Vec3(0.2f, 0.2f, 0.2f), Vec3(0.8f, 0.8f, 0.8f),
        Vec3(1.0f, 1.0f, 1.0f), Vec3(0.0f,  0.0f, -3.0f),
        1, 0.09f, 0.032f);
    pointLight = std::get<PointLight*>(pointLight3->light);
    engine.AddObject<>(pointLight3);

    Object* dirLight = new Object();
    dirLight->light = new DirLight(
            Vec3(0.05f, 0.05f, 0.05f), Vec3(0.4f, 0.4f, 0.4f),
            Vec3(0.5f, 0.5f, 0.5f),  Vec3(-0.2f, -1.0f, -0.3f));

    auto directionLight = std::get<DirLight*>(dirLight->light);
    engine.AddObject<>(dirLight);

    Object* spotLight = new Object();
    spotLight->light = new SpotLight(
            Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f),
            Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f),
            1.0f, 0.09f, 0.032f, Vec3(0),
            glm::cos(glm::radians(12.5f)),
            glm::cos(glm::radians(15.0f)));
    auto sptLight = std::get<SpotLight*>(spotLight->light);
    engine.AddObject<>(spotLight);
}
