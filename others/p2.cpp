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
  float growth = 1.0;

  void update(double dt) {
    vel += acc * dt;
    pos += vel * dt;
  }

};

struct MyApp : public App {
  //UI
  ParameterInt HunterPopulation{"/HunterPopulation", "", 10, 2, 100}; // Number of hunters
  ParameterInt PreyPopulation{"/PreyPopulation", "", 10, 2, 100}; // Number of prey
  ParameterColor color{"/color"}; // Background color

  // P
  std::vector<Particle> hunters;
  std::vector<Particle> preys;
  Mesh hunterMesh, preyMesh;
  Light hunterLight, preyLight;
  Material hunterMaterial, preyMaterial;


  void onInit() override {
    // ***** UI *****
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(HunterPopulation);
    gui.add(PreyPopulation);
    gui.add(color);
  }

  void onCreate() override {
    // Sphere Hunter
    addSphere(hunterMesh);
    hunterMesh.scale(0.03);
    hunterMesh.generateNormals();

    // Sphere Prey
    addCube(preyMesh);
    preyMesh.scale(0.02);
    preyMesh.generateNormals();

    // Light and view
    nav().pos(0, 0, 6);
    hunterLight.pos(-2, 7, 0);
    preyLight.pos(2, 7, 0);

  }
  
  void reset(int PreyCount, int HunterCount) {
    hunters.clear();
    preys.clear();
    hunters.resize(HunterCount); // n agents
    preys.resize(PreyCount); // n agents


    // Initialize ecosystem 
    for (auto& h : hunters) {
      h.pos = Vec3f(rs(), rs(), rs());
      h.vel = Vec3f(0.1, 0.1, 0.1);
      h.growth = 1.0;
    }

    for (auto& p : preys) {
      p.pos = Vec3f(rs(), rs(), rs());
      p.vel = Vec3f(0.1, 0.1, 0.1);
      p.growth = 1.0;
    }
  }

  int lastN = 0;

  void onAnimate(double dt) override {
    if (PreyPopulation != lastN) {
      lastN = PreyPopulation;
      reset(PreyPopulation, HunterPopulation);
    }

    // Move the hunters toward the prey
    for (int i = 0; i < hunters.size(); i++) {
      auto& me = hunters[i];

      //hunt the closest prey
      float closestDistance = 10000.0;
      int targetPrey = 0;

      for (int j = 0; j < preys.size(); j++) {
        auto& somePrey = preys[j];
        float preyTargetDistance = length(me.pos - somePrey.pos);
        if (preyTargetDistance < closestDistance) {
          closestDistance = preyTargetDistance;
          int targetPrey = j;
        } 
      }

      if (closestDistance < 0.05) {
        // If the hunter is close to the prey, reduce speed and grow the hunter
        me.vel *= 0.05; // Reduce speed when close to a hunter
        me.growth += 0.02; // grow the hunter

        preys[targetPrey].pos = Vec3f(rs(), rs(), rs()); // Respawn the eaten prey
        preys[targetPrey].vel = Vec3f(0.1, 0.1, 0.1); // Reset prey velocity

      } else {
        // Otherwise, move toward the prey
        Vec3f direction = preys[targetPrey].pos - me.pos;
        direction /= length(direction); // Normalize the direction vector
        me.vel += direction * 0.1; // Move toward the prey
      }
    }

    for (int i = 0; i < preys.size(); i++) {
      auto& me = preys[i];

      // Flee from the closest hunter
      float closestDistance = 10000.0;
      int targetHunter = 0;

      for (int j = 0; j < hunters.size(); j++) {
        auto& someHunter = hunters[j];
        float hunterTargetDistance = length(me.pos - someHunter.pos);
        if (hunterTargetDistance < closestDistance) {
          closestDistance = hunterTargetDistance;
          targetHunter = j;
        }
      }
      if (closestDistance < 0.1) {
        // If the prey is close to the hunter, flee
        Vec3f direction = me.pos - hunters[targetHunter].pos;
        direction /= length(direction); // Normalize the direction vector
        me.vel += direction * 0.1; // Move away from the hunter
      }
    }

    for (auto& a : hunters) {
      a.update(dt);
    }

    for (auto& a : preys) {
      a.update(dt);
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(color);

    // Hunter setup
    hunterLight.ambient(RGB(0));         
    hunterLight.diffuse(RGB(1, 1, 0.5)); 
    g.lighting(true);
    g.light(hunterLight);

    hunterMaterial.specular(hunterLight.diffuse() * 0.2); 
    hunterMaterial.shininess(50); 
    g.material(hunterMaterial);

    // Prey setup
    preyLight.ambient(RGB(0));         
    preyLight.diffuse(RGB(1, 1, 0.5)); 
    g.lighting(true);
    g.light(preyLight);

    preyMaterial.specular(preyLight.diffuse() * 0.2); 
    preyMaterial.shininess(50); 
    g.material(preyMaterial);

    //Draw the Hunters
    for (auto& h : hunters) {
      g.pushMatrix();
      g.translate(h.pos);
      g.scale(h.growth); // Scale the hunter mesh based on growth
      g.color(HSV(0.2));
      g.draw(hunterMesh);
      g.popMatrix();
    }

    // Draw the Preys
    for (auto& a : preys) {
      g.pushMatrix();
      g.translate(a.pos);
      g.draw(preyMesh);
      g.popMatrix();
    }
  }

  bool onKeyDown(const Keyboard& k) override {
    if (k.key() == 'r') {
      reset(PreyPopulation, HunterPopulation);
      for (auto& h : hunters) {
        h.growth = 1.0; // Reset hunter growth
      }
      for (auto& p : preys) {
        p.growth = 1.0; // Reset prey growth
      }
    }
    return true;
  }


};

int main() { MyApp().start(); }
