// Karl Yerkes
// 2022-01-20

#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

using namespace al;

#include <algorithm>
#include <fstream>
#include <random>
#include <vector>

using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

string slurp(string fileName);  // forward declaration

const int N = 50;

struct WorldState {
  double time;
  int frame;
  Pose camera;
  Vec3f position[N];
  Color color[N];
};

struct AlloApp : DistributedAppWithState<WorldState> {
  Parameter pointSize{"/pointSize", "", 2.0, 1.0, 10.0};
  Parameter timeStep{"/timeStep", "", 0.03, 0.01, 0.6};
  Parameter dragFactor{"/dragFactor", "", 0.8, 0.0, 0.9};
  Parameter sphereRadius{"/sphereRadius", "", 1.0, 0.1, 5.0};
  Parameter repulsionFromOrigin{"/repulsionFromOrigin", "", 0.6, 0.0, 5.0};
  Parameter chargePerParticle{"/chargePerParticle", "", 0.0, 0.0, 5.0};
  Parameter lovePower{"/lovePower", "", 0.0, 0.0, 2.0};

  ShaderProgram pointShader;

  //  simulation state
  Mesh particles;  // position *is inside the mesh* mesh.vertices() are the
                   // positions
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;
  vector<int> lovers;

  void onInit() override {
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<WorldState>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    if (isPrimary()) {
      // set up GUI
      auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto& gui = GUIdomain->newGUI();
      gui.add(pointSize);
      gui.add(timeStep);
      gui.add(dragFactor);
      gui.add(sphereRadius);
      gui.add(repulsionFromOrigin);
      gui.add(chargePerParticle);
      gui.add(lovePower);
    }
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

    for (int _ = 0; _ < N; _++) {
      particles.vertex(randomVec3f(5));
      particles.color(1.0, 1.0, 1.0);  // white

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

    // Love settings
    for (int i = 0; i < velocity.size(); i++) {
      lovers.push_back(i);
    }

    // Initialize random number generator
    std::random_device rd;
    std::mt19937 g(rd());

    // Shuffle the vector
    std::shuffle(lovers.begin(), lovers.end(), g);
    // Print the shuffled vector
    /*
    std::cout << "Lovers: " << endl;
    for (int i = 0; i < lovers.size(); i++) {
        cout << lovers[i] << " ";
      }
    */
  }

  bool freeze = false;
  float k = .3;  // spring constant

  void onAnimate(double dt) override {
    if (!isPrimary()) {
      return;
    }

    state().frame++;
    state().time += dt;

    if (freeze) return;

    // calculate spring force between this particle and the origin
    for (int i = 0; i < velocity.size(); i++) {
      auto& me = particles.vertices()[i];
      Vec3f dist = me.normalized();  // distance from origin

      force[i] += -dist * k;                         // F = -kx
      velocity[i] += force[i] / mass[i] * timeStep;  // F = ma -> a = F/m

      if (me.mag() <= sphereRadius) {
        // if particle is close to the origin, apply a repulsive force
        force[i] += me.normalized() * repulsionFromOrigin;  // F = -b * v
        velocity[i] += force[i] / mass[i] * timeStep;       // F = ma -> a = F/m
      }
    }

    // Repulsive forces  Coulomb's Law: F = k * q1 * q2 / r^2
    // float k_c = 8.987 * (std::pow(10, 9)); // Coulomb's constant
    float k_c = 1.0;  // for simplicity, we can set k_c to 1 in our simulation

    for (int i = 0; i < particles.vertices().size(); ++i) {
      for (int j = i + 1; j < particles.vertices().size(); ++j) {
        if (i == j) continue;  // skip self

        auto& me = particles.vertices()[i];
        auto& other = particles.vertices()[j];

        auto dir = me - other;
        float dist = dir.mag();
        if (dist < 0.3) {
          Vec3f vector = dir.normalize();
          force[i] += k_c * vector * chargePerParticle * chargePerParticle /
                      (dist * dist);
          force[j] -= k_c * vector * chargePerParticle * chargePerParticle /
                      (dist * dist);

          // ignore forces that are too large
          if (force[i].mag() > 10) {
            continue;
          } else {
            velocity[i] += force[i] / mass[i] * timeStep;
          }

          if (force[j].mag() > 10) {
            continue;
          } else {
            velocity[j] += force[j] / mass[j] * timeStep;
          }
        }
      }
    }

    // Love forces
    for (int i = 0; i < velocity.size(); i++) {
      auto& me = particles.vertices()[i];
      auto& loveOne = particles.vertices()[lovers[i]];
      Vec3f dir = loveOne - me;
      Vec3f vector = dir.normalize();

      force[i] += vector * lovePower;  // F = dist * <3
      velocity[i] += force[i] / mass[i] * timeStep;
    }

    // viscous drag
    for (int i = 0; i < velocity.size(); i++) {
      force[i] += -velocity[i] * dragFactor;  // F = -bv
    }

    // Numerical Integration
    vector<Vec3f>& position(particles.vertices());
    for (int i = 0; i < velocity.size(); i++) {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;
    }

    // clear all accelerations (IMPORTANT!!)
    for (auto& a : force) a.set(0);

    for (int i = 0; i < N; i++) {
      state().position[i] = particles.vertices()[i];
    }
    state().camera = nav();
  };

  bool onKeyDown(const Keyboard& k) override {
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

    if (k.key() == 'r') {
      // reset positions and velocities
      for (int i = 0; i < velocity.size(); i++) {
        particles.vertices()[i] = randomVec3f(5);
        velocity[i] = randomVec3f(0.1);
        force[i] = randomVec3f(1);
      }
    }

    return true;
  }

  void onDraw(Graphics& g) override {
    if (!isPrimary()) {
      nav().set(state().camera);
    }

    g.clear(0.0);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);

    if (!isPrimary()) {
      for (int i = 0; i < N; i++) {
        particles.vertices()[i] = state().position[i];
      }
    }
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
