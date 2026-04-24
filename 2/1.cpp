// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName);  // forward declaration


struct AlloApp : App {
  Parameter pointSize{"/pointSize", "", 2.0, 1.0, 10.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Parameter dragFactor{"/dragFactor", "", 0.1, 0.0, 0.9};
  Parameter sphereRadius{"/sphereRadius", "", 1.0, 0.1, 5.0};
  Parameter repulsionStrength{"/repulsionStrength", "", 1.4, 0.0, 5.0};


  ShaderProgram pointShader;

  //  simulation state
  Mesh particles;  // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;

  void onInit() override {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  
    gui.add(timeStep);  
    gui.add(dragFactor);   
    gui.add(sphereRadius);  
    gui.add(repulsionStrength);   

    //
  }

  void onCreate() override {
    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    // set initial conditions of the simulation
    

    // auto randomColor = []() { return HSV(rnd::uniform(), 1.0f, 1.0f); };

    particles.primitive(Mesh::POINTS);

    for (int _ = 0; _ < 500; _++) {
      particles.vertex(randomVec3f(5));
      particles.color(1.0, 1.0, 1.0); // white

      // float m = rnd::uniform(3.0, 0.5);
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5) m = 0.5;
      mass.push_back(m);

      // using a simplified volume/size relationship
      particles.texCoord(pow(m, 1.0f / 3), 0);  // s, t

      // separate state arrays
      velocity.push_back(randomVec3f(0.1));
      force.push_back(randomVec3f(1));
    }

    nav().pos(0, 0, 10);
  }

  bool freeze = false;
  float k = .3; // spring stiffness

  void onAnimate(double dt) override {
    if (freeze) return;

    // calculate spring force between this particle and the origin
    for (int i = 0; i < velocity.size(); i++) {
      auto& me = particles.vertices()[i];
      Vec3f dist = me.normalized(); // distance from origin
      
      force[i] +=  - dist * k; // F = -kx
      velocity[i] += force[i] / mass[i] * timeStep; // F = ma -> a = F/m
      
      if (me.mag() <= sphereRadius) {
        // if particle is close to the origin, apply a repulsive force
        force[i] += me.normalized() * repulsionStrength; // F = -b * v
        velocity[i] += force[i] / mass[i] * timeStep; // F = ma -> a = F/m
      }
        

    }

///////// you probably do not need to edit the rest of this method...

    // viscous drag
    for (int i = 0; i < velocity.size(); i++) {
      force[i] += - velocity[i] * dragFactor; // F = -bv
    }

    
    // Numerical Integration
    vector<Vec3f> &position(particles.vertices());
    for (int i = 0; i < velocity.size(); i++) {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;
    }

    // clear all accelerations (IMPORTANT!!)
    for (auto &a : force) a.set(0);
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      freeze = !freeze;
    }

    if (k.key() == '1') {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++) {
        // F = ma
        force[i] += randomVec3f(1);
      }
    }

    return true;
  }

  void onDraw(Graphics &g) override {
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(particles);
  }
};

int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
