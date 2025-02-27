#include "openglwindow.hpp"
#include <imgui.h>

#include "abcg.hpp"
#include <iostream>
#include <list>
#include <string>

void OpenGLWindow::handleEvent(SDL_Event &event) {
  // Keyboard events
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_z)
      m_gameData.m_input.set(static_cast<size_t>(Input::Shoot));
    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
      m_gameData.m_input.set(static_cast<size_t>(Input::Up));
    if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
      m_gameData.m_input.set(static_cast<size_t>(Input::Down));
    if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
      m_gameData.m_input.set(static_cast<size_t>(Input::Left));
    if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d)
      m_gameData.m_input.set(static_cast<size_t>(Input::Right));
    if (event.key.keysym.sym == SDLK_LSHIFT )
      m_gameData.m_input.set(static_cast<size_t>(Input::Focus));
    if (event.key.keysym.sym == SDLK_r )
      m_gameData.m_input.set(static_cast<size_t>(Input::Restart));
  }
  if (event.type == SDL_KEYUP) {
    if (event.key.keysym.sym == SDLK_z)
      m_gameData.m_input.reset(static_cast<size_t>(Input::Shoot));
    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
      m_gameData.m_input.reset(static_cast<size_t>(Input::Up));
    if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
      m_gameData.m_input.reset(static_cast<size_t>(Input::Down));
    if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
      m_gameData.m_input.reset(static_cast<size_t>(Input::Left));
    if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d)
      m_gameData.m_input.reset(static_cast<size_t>(Input::Right));
    if (event.key.keysym.sym == SDLK_LSHIFT )
      m_gameData.m_input.reset(static_cast<size_t>(Input::Focus));
    if (event.key.keysym.sym == SDLK_r )
      m_gameData.m_input.reset(static_cast<size_t>(Input::Restart)); 
  }
  

}

void OpenGLWindow::initializeGL() {
  
  // Create program to render the other objects
  m_objectsProgram = createProgramFromFile(getAssetsPath() + "objects.vert",
                                           getAssetsPath() + "objects.frag");

  m_starsProgram = createProgramFromFile(getAssetsPath() + "stars.vert",
                                           getAssetsPath() + "stars.frag");

  m_bulletsProgram = createProgramFromFile(getAssetsPath() + "objects.vert",
                                           getAssetsPath() + "bullets.frag");

  glClearColor(0, 0, 0, 1);

#if !defined(__EMSCRIPTEN__)
  glEnable(GL_PROGRAM_POINT_SIZE);
#endif

  // Start pseudo-random number generator
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);

  restart();
}

void OpenGLWindow::restart() {
  stage = Stage1();
  stage.m_bulletsProgram = m_bulletsProgram;
  stage.m_objectsProgram = m_objectsProgram;
  defeated = 0;
  m_gameData.m_state = State::Playing;
  m_gameData.m_state = State::Playing;
  m_restartWaitTimer.restart();
  m_starLayers.initializeGL(m_starsProgram, 25);
  m_player.initializeGL(m_objectsProgram);
  m_player.m_lives = 3;
  m_enemies.clear();
  m_player.m_bullets.clear();
  m_eBullets.clear();

  // m_eBullets.push_back(EnemyBullet{});
  // m_eBullets.back().initializeGL(m_bulletsProgram);
  
  

  // m_enemies.push_back(Enemy{});

  std::vector<shootPattern> shoots{};
  std::vector<uint16_t> indexes{};

  indexes.push_back(0);

  shoots.push_back(shootPattern{});
  

  for(auto &enemy : m_enemies){
    // enemy.setBehavior(m_objectsProgram, 100, std::vector<glm::vec2>{glm::vec2{-1.0f, 1.0f },glm::vec2{-0.6f, -0.5f},glm::vec2{0.0f, 0.5f},glm::vec2{-1.0f, 1.0f }},
    //                   std::vector<float>{0.5f,10.8f,0.8f}, std::vector<float>{1.5f,1.5f,1.5f});
    enemy.createEnemy(m_objectsProgram, 100, std::vector<glm::vec2>{glm::vec2{-1.0f, 1.0f },glm::vec2{-0.6f, -0.5f},glm::vec2{0.0f, 0.5f},glm::vec2{-1.0f, 1.0f }},
                      std::vector<float>{0.5f,5.0f,0.8f}, std::vector<float>{1.5f,1.5f,1.5f},m_bulletsProgram , shoots, indexes);
  }
}

void OpenGLWindow::update() {
  float deltaTime{static_cast<float>(getDeltaTime())};

  if(m_restartWaitTimer.elapsed() > 1.0f && m_gameData.m_input[static_cast<size_t>(Input::Restart)]){
    restart();
    return;
  }

  stage.update(m_enemies);

  for(auto &enemy : m_enemies){
    enemy.update(m_player.m_translation, m_gameData, deltaTime, m_eBullets);
  }


  playerPosition = m_player.m_translation;

  for(auto &bullet : m_player.m_bullets){
    bullet.update(m_gameData, deltaTime);
  }

  for(auto &ebullet : m_eBullets){
    ebullet.update(m_gameData, deltaTime);
  }
  
  m_starLayers.update(m_player, deltaTime);

  m_player.update(m_gameData, deltaTime);


  checkCollisions();

  for(auto i = m_player.m_bullets.begin(); i != m_player.m_bullets.end(); ++i){
    if(i->m_hit){
      m_player.m_bullets.erase(i);
      i--;
    }
  }
  for(auto i = m_enemies.begin(); i != m_enemies.end(); ++i){
    if(i->m_destroyed){
      m_enemies.erase(i);
      i--;
    }
  }
  for(auto i = m_eBullets.begin(); i != m_eBullets.end(); ++i){
    if(i->m_hit){
      m_eBullets.erase(i);
      i--;
    }
  }

}

void OpenGLWindow::paintGL() {
  update();

  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  m_starLayers.paintGL();

  m_player.paintGL(m_gameData);
  for(auto &enemy : m_enemies){
    enemy.paintGL(m_gameData);
  }

  

  for(auto &bullet : m_player.m_bullets){
    bullet.paintGL(m_gameData);
  }

  for(auto &ebullet : m_eBullets){
    
    ebullet.paintGL(m_gameData);
  }  
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  {
    auto size{ImVec2(300, 85)};
    auto position{ImVec2((m_viewportWidth - size.x)+50.0f,
                         (m_viewportHeight - size.y) / 2.0f)};
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags flags{ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoInputs};
    ImGui::Begin(" ", nullptr, flags);
    ImGui::Text("Extra lives: %d\nDefeated enemies: %d", m_player.m_lives, defeated);
  
    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;

  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLWindow::terminateGL() {
  glDeleteProgram(m_starsProgram);
  glDeleteProgram(m_objectsProgram);

  m_player.terminateGL();
  m_starLayers.terminateGL();
}

void OpenGLWindow::checkCollisions(){
  
  for(auto i = m_enemies.begin(); i != m_enemies.end(); ++i){
    if(!m_player.m_invincibility && (glm::distance(m_player.m_translation, i->m_translation) < (m_player.m_scale/4) + i->m_scale*0.66f)){
      m_player.damage();

      if(m_player.m_lives < 0){
        m_player.m_lives = 0;
        m_gameData.m_state = State::Defeated;
      }
    }
    for(auto j = m_player.m_bullets.begin(); j != m_player.m_bullets.end(); ++j){
      if(glm::distance(j->m_translation, i->m_translation) < (j->m_scale/2) + i->m_scale){
        i->life--;
        // std::cout << i->life << std::endl;
        if(i->life == 0){
          defeated++;
          i->m_destroyed = true;
        }
        j->m_hit = true;
      }
    }
  }
  for(auto i = m_eBullets.begin(); i != m_eBullets.end(); ++i){
    if(!m_player.m_invincibility && (glm::distance(m_player.m_translation, i->m_translation) < (m_player.m_scale/4) + i->m_scale*0.7f)){
      m_player.damage();
      i->m_hit = true;
      if(m_player.m_lives < 0){
        m_player.m_lives = 0;
        m_gameData.m_state = State::Defeated;
      }
    }
  }
}