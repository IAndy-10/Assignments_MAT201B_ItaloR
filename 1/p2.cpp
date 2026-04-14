/*
## Assignment 2

**Due next Wednesday by 5**

**Read:**

- `Paper/1987/Reynolds/Flocks, Herds, and Schools ~ A Distributed Behavioral Model.pdf`
- `Paper/2014/Kuchera-Morin/Immersive full-surround multi-user system design.pdf`

Reflect on these readings. Share a Google Doc with `yerkes@ucsb.edu` that presents your experience and questions for me.

**Code:**

Implement an interesting agent simulation in AlloLib. 
Use `al::Nav` as the basis of each agent. 
1. Implement *random chasing* where each agent "loves" some other random agent, following that agent wherever it goes
    1) turn a little toward your love
    2) move in the forward direction.
Call this code `p1.cpp`. 

Next, add code that maintains distance between agents. 
For each agent, search through all the other agents to find those that are closer than some threshold value. 
Nudge away from these too close neighbors. Call this code `p2.cpp`. 

Next, abandon love as a concept. Instead, make the agents "flock" by 
    1) adjusting their heading to match the average heading of their neighbors
    2) maintaining a small distance from their neighbors, while 
    3) maintaining proximity to their neighbors by nudging toward their average position. 
    For each agent, find up to K neighbors closer than some threshold distance T. 
    Compute the average position of the neighbors and their average heading. 
    If the agent is too far away from the center of the neighbors, nudge toward the center. 
    If the agent is too close to any neighbor, nudge away from that neighbor. 
    
Turn a little to match the average heading. 
The agents may wander off into the distance, in different directions, so implement [wrap around](https://en.wikipedia.org/wiki/Wraparound_(video_games)). 
Call this code `p3.cpp`. Now add something interesting of your own design. Perhaps:

- Introduce food into the system
- Add obstacles
- Find some way to keep agents contained without wrap around
- Add lions that eat flocking agents
- Make agents fight for survival
- Give agents different traits: size, speed, agility, color
- Give agents flexible bodies
- ??

Call this code `p4.cpp`.

You will turn these in with git on GitHub.
You will need to know the methods of `al::Nav` and `al::Vec3x` and you will need to learn how to use `std::vector<>` and `for` loops.
*/


#include <iostream>

#include "al/app/al_App.hpp"  // al::App
#include "al/graphics/al_Shapes.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

float r() { return rnd::uniform(); } // random float in range [0, 1]
float rs() { return rnd::uniformS(); } // random float in range [-1, 1]

struct MyApp : public App {
  //Parameters
  ParameterInt N{"/N", "", 10, 2, 100};
  Parameter neighbor_distance{"/n", "", 0.01, 0.01, 3};
  ParameterColor color{"/color"};


  Light light;
  Material material;

  Mesh mesh;

  std::vector<Nav> agent;
  std::vector<Nav> loves;

  void onInit() override {
    // UI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(N);
    gui.add(neighbor_distance);
    
  }

  void onCreate() override {
    // Cone Agent
    addCone(mesh);
    mesh.scale(1, 0.2, 1);
    mesh.scale(0.2);
    mesh.generateNormals();

    // Light and view
    nav().pos(0, 0, 6);
    light.pos(-2, 7, 0);
  }
  
  void reset(int n) {
    agent.clear();
    agent.resize(n); // n agents
    loves.clear();
    loves.resize(n);
    for (auto& a : agent) {
      a.pos(Vec3d(rs(), rs(), rs()));
    }
  }



  int lastN = 0;

  void onAnimate(double dt) override {
    if (N != lastN) {
      lastN = N;
      reset(N);
    }
    // Love setting
    loves.resize(agent.size());
    for (int i = 0; i < agent.size(); i++) {
      if (i == agent.size()-1) {
          loves[i] = agent[0];
      } else {
          loves[i] = agent[i+1];
      }
    }


    for (int i = 0; i < agent.size(); i++) {
      auto& me = agent[i];
      auto& myTrueLove = loves[i];
      me.faceToward(myTrueLove.pos(), 0.05);
      me.moveF(0.7);

      Vec3d sum;
      int count = 0;

      for (int j = 0; j < agent.size(); j++) {
        if (i == j) {
          continue;
        }
        // Neighboring agent
        auto& them = agent[j];

        float distance = (me.pos() - them.pos()).mag();
        if (distance < neighbor_distance) {
          count++;
          sum += them.pos();
          sum += me.pos();
        }
      }
      
      if (count > 1) {
        Vec3d center = sum / (count + 1);
        float distanceToCenter = (me.pos() - center).mag();
        me.faceToward(center, 0.05);
        me.moveF(-0.7);
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
