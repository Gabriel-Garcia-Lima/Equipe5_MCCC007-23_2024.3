#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include "abcg.hpp"
#include "abcgOpenGL.hpp"
#include <glm/vec2.hpp>
#include <random>
#include <vector>

class Window final : public abcg::OpenGLWindow {
public:
  ~Window() = default;

  // Making Flipper public
  struct Flipper {
  glm::vec2 position{};
  float currentAngle{};
  float targetAngle{};
  float angularVelocity{};
  float length{};
};

  struct Obstacle {
    glm::vec2 position;
    float radius;
  };

private:
  GLuint m_program{};
  GLint m_colorLoc{};
  GLint m_translateLoc{};
  GLint m_scaleLoc{};
  GLint m_rotateLoc{};
  GLuint m_VAO{};

  std::vector<Obstacle> m_obstacles;

  struct Ball {
    glm::vec2 position{};
    glm::vec2 velocity{};
    float radius{};
  };

  // Game objects
  Ball m_ball;
  Flipper m_leftFlipper;
  Flipper m_rightFlipper;

  // Game state
  float m_gameScale{0.15f};
  bool m_gameStarted{false};

  void onCreate() override;
  void onUpdate() override;
  void onPaint() override;
  void onDestroy() override;
  void onEvent(SDL_Event const &event) override;

  void setupBall();
  // void setupObstacles();
  void setupFlippers();
  void checkCollisions();
  void checkWallCollision();
  void checkBottomWallCollision();
  void renderBall();
  void renderFlipper(Flipper const &flipper, bool isLeft);
  void renderWalls();
  void renderObstacles(glm::vec2 const &position, float radius);
  void checkCollisionWithObstacles();
};

#endif
