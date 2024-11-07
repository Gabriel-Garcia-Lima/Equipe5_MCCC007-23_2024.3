#include "window.hpp"
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

constexpr float WALL_LEFT = -1.0f;
constexpr float WALL_RIGHT = 1.0f;
constexpr float WALL_BOTTOM = -1.0f;
constexpr float WALL_TOP = 1.0f;

void Window::onCreate() {
  // Vertex and Fragment shaders to handle transformations and color
  auto const *vertexShader = R"gl(
    #version 300 es
    layout(location = 0) in vec2 inPosition;
    uniform vec2 translate;
    uniform float scale;
    uniform float rotate;
    void main() {
      vec2 pos = inPosition;
      float sinTheta = sin(rotate);
      float cosTheta = cos(rotate);
      vec2 rotPos = vec2(
        pos.x * cosTheta - pos.y * sinTheta,
        pos.x * sinTheta + pos.y * cosTheta
      );
      vec2 finalPos = rotPos * scale + translate;
      gl_Position = vec4(finalPos, 0, 1);
    }
  )gl";

  auto const *fragmentShader = R"gl(
    #version 300 es
    precision mediump float;
    uniform vec4 color;
    out vec4 outColor;
    void main() { outColor = color; }
  )gl";

  // Initialize random number generator
  // Initialize random number generator
  std::random_device rd;
  std::mt19937 gen(rd());

  // Ensure obstacles are not near walls
  std::uniform_real_distribution<float> distPosX(WALL_LEFT + 0.2f,
                                                 WALL_RIGHT - 0.2f);

  // Only the upper half of the screen (and allow for some margin from the top)
  std::uniform_real_distribution<float> distPosY(-0.2f, WALL_TOP - 0.2f);

  // Increase the range for a larger obstacle radius
  std::uniform_real_distribution<float> distRadius(
      0.1f, 0.3f); // New range for larger obstacles (adjust the max as needed)

  // Randomly generate obstacles
  int numObstacles = 6; // Number of obstacles, can be adjusted
  m_obstacles.clear();  // Clear any previous obstacles

  for (int i = 0; i < numObstacles; ++i) {
    glm::vec2 randomPosition(
        distPosX(gen), distPosY(gen));    // Random position within the top half
    float randomRadius = distRadius(gen); // Random radius with larger range

    // Add the new obstacle to the list
    m_obstacles.push_back({randomPosition, randomRadius});
  }

  m_program = abcg::createOpenGLProgram(
      {{.source = vertexShader, .stage = abcg::ShaderStage::Vertex},
       {.source = fragmentShader, .stage = abcg::ShaderStage::Fragment}});

  m_colorLoc = glGetUniformLocation(m_program, "color");
  m_translateLoc = glGetUniformLocation(m_program, "translate");
  m_scaleLoc = glGetUniformLocation(m_program, "scale");
  m_rotateLoc = glGetUniformLocation(m_program, "rotate");

  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // setupObstacles();

  setupBall();
  setupFlippers();

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glLineWidth(10.0f);
}

// void Window::setupObstacles() {
//   // Add some obstacles (circles) inside the play area
//   m_obstacles.push_back({glm::vec2(0.0f, 0.2f), 0.1f});   // Example obstacle
//   1 m_obstacles.push_back({glm::vec2(-0.3f, 0.5f), 0.15f}); // Example
//   obstacle 2 m_obstacles.push_back({glm::vec2(0.3f, -0.3f), 0.12f}); //
//   Example obstacle 3
// }

// Function to render obstacles as white circles
void Window::renderObstacles(glm::vec2 const &position, float radius) {
  glBindVertexArray(m_VAO);

  static const int numTriangles = 20;
  std::vector<glm::vec2> positions;
  positions.emplace_back(0, 0); // Center of the circle

  // Generate points in a circular arrangement
  for (int i = 0; i <= numTriangles; i++) {
    auto const angle = i * M_PI * 2.0f / numTriangles;
    positions.emplace_back(radius * std::cos(angle), radius * std::sin(angle));
  }

  // Set the obstacle color to white
  glUniform4f(m_colorLoc, 1.0f, 1.0f, 1.0f, 1.0f); // White color

  // Set position of the obstacle
  glUniform2f(m_translateLoc, position.x, position.y);
  glUniform1f(m_rotateLoc, 0.0f);
  glUniform1f(m_scaleLoc, m_gameScale);

  GLuint VBO{};
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
               positions.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, positions.size());

  glDeleteBuffers(1, &VBO);
}

void Window::checkCollisionWithObstacles() {
  float scaledBallRadius = m_ball.radius * m_gameScale;

  for (auto const &obstacle : m_obstacles) {
    // Calculate the distance from the ball center to the obstacle center
    float distance = glm::length(m_ball.position - obstacle.position);

    // Check if the ball's radius overlaps with the obstacle's radius
    if (distance < (scaledBallRadius + obstacle.radius/2)) {
      // Simple bounce logic: Reflect the ball's velocity
      glm::vec2 normal = glm::normalize(m_ball.position - obstacle.position);
      m_ball.velocity = glm::reflect(m_ball.velocity, normal);

      // Optionally, you could adjust the ball's position to prevent it from
      // getting stuck inside the obstacle
      // m_ball.position =
      //     obstacle.position + normal * (scaledBallRadius + obstacle.radius);
    }
  }
}

void Window::setupBall() {
  // Start the ball outside the play area in a top-right corridor
  m_ball.position = {1.1f, 0.8f}; // Position outside the right wall
  m_ball.velocity = {0.0f, 0.0f}; // Initially stationary
  m_ball.radius = 0.20f;
}

void Window::setupFlippers() {
  m_leftFlipper.position = {-0.5f, -0.8f};
  m_leftFlipper.currentAngle = -0.3f;
  m_leftFlipper.targetAngle = -0.3f;
  m_leftFlipper.length = 0.4f;

  m_rightFlipper.position = {0.5f, -0.8f};
  m_rightFlipper.currentAngle = -0.3f;
  m_rightFlipper.targetAngle = -0.3f;
  m_rightFlipper.length = 0.4f;
}

void Window::renderFlipper(Flipper const &flipper, bool isLeft) {
  glBindVertexArray(m_VAO);


  const float flipperHalfHeight = 0.025f;

  std::array<glm::vec2, 4> positions;
  if (isLeft) {
    positions = {
      glm::vec2{0.0f, -flipperHalfHeight},
      glm::vec2{flipper.length/m_gameScale, -flipperHalfHeight},
      glm::vec2{flipper.length/m_gameScale, flipperHalfHeight},
      glm::vec2{0.0f, flipperHalfHeight}
    };
  } else {
    positions = {
      glm::vec2{0.0f, -flipperHalfHeight},
      glm::vec2{-flipper.length/m_gameScale, -flipperHalfHeight},
      glm::vec2{-flipper.length/m_gameScale, flipperHalfHeight},
      glm::vec2{0.0f, flipperHalfHeight}
    };
  }

  glUniform4f(m_colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
  glUniform2f(m_translateLoc, flipper.position.x, flipper.position.y);
  float angle = isLeft ? flipper.currentAngle : -flipper.currentAngle;
  glUniform1f(m_rotateLoc, angle);
  glUniform1f(m_scaleLoc, m_gameScale);

  // Criar e enviar o VBO
  GLuint VBO{};
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
               positions.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  // Desenhar o flipper
  glDrawArrays(GL_TRIANGLE_FAN, 0, positions.size());

  // Limpar recursos
  glDeleteBuffers(1, &VBO);
}

void Window::onUpdate() {
  if (!m_gameStarted)
    return;

  auto const deltaTime{getDeltaTime()};

  // Evitar divisão por zero
  if (deltaTime <= std::numeric_limits<float>::epsilon()) return;

  // Atualiza ângulos dos flippers
  float maxAngularSpeed = 5.0f; // Velocidade angular máxima (radianos por segundo)
  float maxAngularVelocity = 5.0f; // Velocidade angular máxima permitida

  auto updateFlipperAngle = [&](Flipper &flipper) {
    float previousAngle = flipper.currentAngle;

    // Atualiza o ângulo atual em direção ao ângulo alvo
    float angleDifference = flipper.targetAngle - flipper.currentAngle;
    float angleStep = maxAngularSpeed * deltaTime;

    if (std::abs(angleDifference) < angleStep) {
      flipper.currentAngle = flipper.targetAngle;
    } else {
      flipper.currentAngle += (angleDifference > 0 ? angleStep : -angleStep);
    }

    // Calcula a velocidade angular
    if (deltaTime > std::numeric_limits<float>::epsilon()) {
      flipper.angularVelocity = (flipper.currentAngle - previousAngle) / deltaTime;

      // Limita a velocidade angular máxima
      if (flipper.angularVelocity > maxAngularVelocity)
        flipper.angularVelocity = maxAngularVelocity;
      else if (flipper.angularVelocity < -maxAngularVelocity)
        flipper.angularVelocity = -maxAngularVelocity;
    } else {
      flipper.angularVelocity = 0.0f;
    }
  };

  updateFlipperAngle(m_leftFlipper);
  updateFlipperAngle(m_rightFlipper);
  // Atualiza posição da bola
  m_ball.velocity.y -= 0.8f * deltaTime; // Gravidade

  // Verifica se a velocidade da bola é válida
  if (glm::any(glm::isnan(m_ball.velocity)) || glm::any(glm::isinf(m_ball.velocity))) {
    m_ball.velocity = glm::vec2(0.0f);
  }

  m_ball.position += m_ball.velocity * static_cast<float>(deltaTime);
  checkCollisions();
  checkCollisionWithObstacles();
}

void Window::onEvent(SDL_Event const &event) {
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_SPACE && !m_gameStarted) {
      m_gameStarted = true;
      m_ball.velocity = {2.0f, 0.5f}; // Launch upwards
    }
    // Existing code for flipper controls
    if (event.key.keysym.sym == SDLK_LEFT)
      m_leftFlipper.targetAngle = 0.8f;
    if (event.key.keysym.sym == SDLK_RIGHT)
      m_rightFlipper.targetAngle = 0.8f;
  } else if (event.type == SDL_KEYUP) {
    // Reset flipper angles when keys are released
    if (event.key.keysym.sym == SDLK_LEFT)
      m_leftFlipper.targetAngle = -0.5f;
    if (event.key.keysym.sym == SDLK_RIGHT)
      m_rightFlipper.targetAngle = -0.5f;
  }
}

void Window::checkCollisions() {
  const float flipperHalfHeight = 0.025f;
  // Define the lambda for ball-flipper collision detection
    auto checkBallFlipperCollision = [&](Ball &ball, Flipper const &flipper, bool isLeft) {
    glm::vec2 relPos = ball.position - flipper.position;

    float angle = isLeft ? flipper.currentAngle : -flipper.currentAngle;
    glm::vec2 rotatedPos = glm::rotate(relPos, -angle);

    float xMin = isLeft ? 0.0f : -flipper.length;
    float xMax = isLeft ? flipper.length : 0.0f;

    if (rotatedPos.x >= xMin && rotatedPos.x <= xMax &&
        rotatedPos.y >= -flipperHalfHeight && rotatedPos.y <= flipperHalfHeight) {
      
      // Calcula o vetor normal
      glm::vec2 normal = glm::rotate(glm::vec2(0.0f, 1.0f), angle);

      // Calcula a velocidade do flipper no ponto de contato
      // Evitar valores extremos ou inválidos
      glm::vec2 flipperVelocity = flipper.angularVelocity * glm::vec2(-relPos.y, relPos.x);

      // Limitar a magnitude da velocidade do flipper
      float maxFlipperSpeed = 2.0f; // Ajuste conforme necessário
      if (glm::length(flipperVelocity) > maxFlipperSpeed) {
        flipperVelocity = glm::normalize(flipperVelocity) * maxFlipperSpeed;
      }

      // Ajusta a velocidade da bola
      glm::vec2 relativeVelocity = ball.velocity - flipperVelocity;
      glm::vec2 newVelocity = glm::reflect(relativeVelocity, normal) + flipperVelocity;

      // Verifica se a nova velocidade é válida
      if (glm::any(glm::isnan(newVelocity)) || glm::any(glm::isinf(newVelocity))) {
        newVelocity = glm::vec2(0.0f);
      }

      ball.velocity = newVelocity;

      // Opcional: Adiciona um impulso extra limitado
      // float impulseStrength = 0.02f; // Ajuste conforme necessário
      // ball.velocity += normal * impulseStrength;
    }
  };

  // Chame a função de colisão para ambos os flippers
  checkBallFlipperCollision(m_ball, m_leftFlipper, true);
  checkBallFlipperCollision(m_ball, m_rightFlipper, false);

  checkWallCollision();
  checkBottomWallCollision();

  float scaledBallRadius = m_ball.radius * m_gameScale;
  if ((m_ball.position.y - scaledBallRadius) < WALL_BOTTOM) {
    setupBall();           // Reset the ball position
    m_gameStarted = false; // Require pressing space to relaunch
  }
}

// Function to check collision with the left, right, and top walls
void Window::checkWallCollision() {
  float scaledBallRadius = m_ball.radius * m_gameScale;

  // Check left wall collision
  if ((m_ball.position.x - scaledBallRadius) < (WALL_LEFT + 0.1f)) {
    m_ball.position.x = (WALL_LEFT + 0.1f) + scaledBallRadius;
    m_ball.velocity.x *= -0.8f;
  }

  // Check right wall collision
  if ((m_ball.position.x + scaledBallRadius) > (WALL_RIGHT - 0.1f)) {
    m_ball.position.x = (WALL_RIGHT - 0.1f) - scaledBallRadius;
    m_ball.velocity.x *= -0.8f;
  }

  // Check top wall collision
  if ((m_ball.position.y + scaledBallRadius) > WALL_TOP) {
    m_ball.position.y = WALL_TOP - scaledBallRadius;
    m_ball.velocity.y *= -0.8f;
  }
}

void Window::checkBottomWallCollision() {
  float scaledBallRadius = m_ball.radius * m_gameScale;

  // Left bottom wall (near left flipper)
  if ((m_ball.position.y - scaledBallRadius) < (WALL_BOTTOM + 0.2f) &&
      (m_ball.position.x - scaledBallRadius) < (m_leftFlipper.position.x)) {
    m_ball.position.y = WALL_BOTTOM + 0.2f + scaledBallRadius;
    m_ball.velocity.y *= -0.8f;
  }

  // Right bottom wall (near right flipper)
  if ((m_ball.position.y - scaledBallRadius) < (WALL_BOTTOM + 0.2f) &&
      (m_ball.position.x + scaledBallRadius) > (m_rightFlipper.position.x)) {
    m_ball.position.y = WALL_BOTTOM + 0.2f + scaledBallRadius;
    m_ball.velocity.y *= -0.8f;
  }
}

void Window::onPaint() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(m_program);

  glUniform1f(m_scaleLoc, m_gameScale);
  renderWalls();
  renderBall();
  renderFlipper(m_leftFlipper, true);
  renderFlipper(m_rightFlipper, false);
  // Render obstacles
  for (auto const &obstacle : m_obstacles) {
    renderObstacles(obstacle.position, obstacle.radius);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

// Render the ball as a filled circle
void Window::renderBall() {
  glBindVertexArray(m_VAO);

  static const int numTriangles = 20;
  std::vector<glm::vec2> positions;
  positions.emplace_back(0, 0); // Center of the circle

  // Generate points in a circular arrangement
  for (int i = 0; i <= numTriangles; i++) {
    auto const angle = i * M_PI * 2.0f / numTriangles;
    positions.emplace_back(m_ball.radius * std::cos(angle),
                           m_ball.radius * std::sin(angle));
  }

  // Set the ball color and transformation uniforms
  glUniform4f(m_colorLoc, 1.0f, 0.0f, 0.0f, 1.0f); // Red color
  glUniform2f(m_translateLoc, m_ball.position.x, m_ball.position.y);
  glUniform1f(m_rotateLoc, 0.0f);
  glUniform1f(m_scaleLoc, m_gameScale);

  GLuint VBO{};
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
               positions.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, positions.size());

  glDeleteBuffers(1, &VBO);
}

void Window::renderWalls() {
  glBindVertexArray(m_VAO);

  // Define vertices for walls and boundaries with an opening in the top-right
  std::vector<glm::vec2> positions{
      // Left wall
      {WALL_LEFT, WALL_BOTTOM},
      {WALL_LEFT + 0.1f, WALL_BOTTOM},
      {WALL_LEFT + 0.1f, WALL_TOP},
      {WALL_LEFT, WALL_TOP},

      // Right wall with gap near the top
      {WALL_RIGHT - 0.1f, WALL_BOTTOM},
      {WALL_RIGHT, WALL_BOTTOM},
      {WALL_RIGHT, WALL_TOP - 0.3f}, // Start of gap
      {WALL_RIGHT - 0.1f, WALL_TOP - 0.3f},

      // Resume right wall after gap
      {WALL_RIGHT - 0.1f, WALL_TOP - 0.1f},
      {WALL_RIGHT, WALL_TOP - 0.1f},

      // Top wall
      {WALL_LEFT + 0.1f, WALL_TOP - 0.1f},
      {WALL_RIGHT - 0.1f, WALL_TOP - 0.1f},
  };

  // Add fixed bottom wall sections behind the flippers
  positions.push_back(
      {WALL_LEFT, WALL_BOTTOM - 2.0f}); // Left bottom wall start
  positions.push_back({WALL_LEFT, WALL_BOTTOM + 0.2f}); // Left bottom wall end
  positions.push_back({m_leftFlipper.position.x,
                       m_leftFlipper.position.y}); // Left flipper position

  positions.push_back(
      {WALL_RIGHT, WALL_BOTTOM - 2.0f}); // Right bottom wall start
  positions.push_back(
      {WALL_RIGHT, WALL_BOTTOM + 0.2f}); // Right bottom wall end
  positions.push_back({m_rightFlipper.position.x,
                       m_rightFlipper.position.y}); // Right flipper position

  glUniform4f(m_colorLoc, 0.5f, 0.5f, 0.5f, 1.0f); // Gray color for walls
  glUniform2f(m_translateLoc, 0.0f, 0.0f);
  glUniform1f(m_rotateLoc, 0.0f);
  glUniform1f(m_scaleLoc, 1.0f);

  GLuint VBO{};
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
               positions.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  // Draw each wall section as a separate line loop
  glDrawArrays(GL_LINE_LOOP, 0, 4);  // Left wall
  glDrawArrays(GL_LINE_LOOP, 4, 4);  // Right wall with gap
  glDrawArrays(GL_LINE_LOOP, 8, 4);  // Top wall
  glDrawArrays(GL_LINE_LOOP, 12, 6); // Bottom wall with fixed pit-like opening

  glDeleteBuffers(1, &VBO);
}

// Proper cleanup of OpenGL resources when the window is destroyed
void Window::onDestroy() {
  if (m_VAO != 0)
    glDeleteVertexArrays(1, &m_VAO);
  if (m_program != 0)
    glDeleteProgram(m_program);
}
