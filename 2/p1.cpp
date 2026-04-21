/*
An attractor that attracts agents to a point in space. 
The point is represented by an icosahedron, and the agents are represented by cones. 
The user can control the number of agents.
*/


#include <iostream>

#include "al/app/al_App.hpp"  // al::App
#include "al/graphics/al_Shapes.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

float r() { return rnd::uniform(); } // random float in range [0, 1]
float rs() { return rnd::uniformS(); } // random float in range [-1, 1]

// Class from "gravityWell.cpp" example in AlloLib
class Particle {
 public:
  Vec3f pos;
  Vec3f vel;
  Vec3f acc;

  void update(double dt) {
    vel += acc * dt;
    pos += vel * dt;
  }
};

struct MyApp : public App {
  //UI
  ParameterInt N{"/N", "", 10, 2, 100}; // Number of agents
  ParameterColor color{"/color"}; // Background color

  Light light;
  Material material;

  Mesh mesh;
  Mesh body1;

  std::vector<Nav> agent;
  Particle icosahedron; // for attracting agents


  void onInit() override {
    // ***** UI *****
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(N);
    gui.add(color);
  }

  void onCreate() override {
    // Cone Agent
    addCone(mesh);
    mesh.scale(1, 0.2, 1);
    mesh.scale(0.2);
    mesh.generateNormals();

    // Gravity Icosahedron
    addIcosahedron(body1, 0.1);
    body1.generateNormals();

    // Light and view
    nav().pos(0, 0, 6);
    light.pos(-2, 7, 0);
  }
  
  void reset(int n) {
    agent.clear();
    agent.resize(n); // n agents

    // Initialize icosahedron 
    icosahedron.pos = Vec3f(rs(), rs(), rs());
    icosahedron.vel = Vec3f(0, 0, 0);

    // Initialize agents
    for (auto& a : agent) {
      a.pos(Vec3d(rs(), rs(), rs()));
      a.faceToward(icosahedron.pos);
    }

  }

  int lastN = 0;

  void onAnimate(double dt) override {
    if (N != lastN) {
      lastN = N;
      reset(N);
    }

    // Move the agent toward the icosahedron
    for (int i = 0; i < agent.size(); i++) {
      auto& me = agent[i];
      float distance = length(me.pos() - icosahedron.pos);
      if (distance < 0.05) {
        // If the agent is close to the icosahedron, stop
        me.moveF(0);
      } else {
        // Otherwise, move toward the icosahedron
        me.faceToward(icosahedron.pos);
        me.moveF(0.2);
      }
    }



    for (auto& a : agent) {
      a.step(dt);
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(color);
    light.ambient(RGB(0));          // Ambient reflection for this light
    light.diffuse(RGB(1, 1, 0.5));  // Light scattered directly from light
    g.lighting(true);
    g.light(light);
    material.specular(light.diffuse() * 0.2);  // Specular highlight, "shine"
    material.shininess(50);  // Concentration of specular component [0,128]

    g.material(material);

    //Draw the icosahedron
    g.pushMatrix();
    g.translate(icosahedron.pos);
    g.color(HSV(0.2));
    g.draw(body1);
    g.popMatrix();

    // Draw the agents
    for (auto& a : agent) {
      g.pushMatrix();
      g.translate(a.pos());
      g.rotate(a.quat());
      g.draw(mesh);
      g.popMatrix();
    }
  }
};

int main() { MyApp().start(); }
