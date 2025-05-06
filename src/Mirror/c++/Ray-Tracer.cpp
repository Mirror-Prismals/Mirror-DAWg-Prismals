
// A multi-threaded ray tracer (over 500 lines)!

#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>
#include <algorithm>

using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::thread;
using std::mutex;
using std::lock_guard;
using std::atomic;
using std::cerr;
using std::endl;
using std::min;
using std::max;
using std::ofstream;
using std::cout;

// Constants
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions
inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double clamp(double x, double min_val, double max_val) {
    if(x < min_val) return min_val;
    if(x > max_val) return max_val;
    return x;
}

double random_double() {
    // Returns a random real in [0,1).
    static thread_local std::mt19937 generator((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

// vec3 Class (3D Vector)
class vec3 {
public:
    double e[3];

    vec3() : e{0, 0, 0} {}
    vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3 &v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(const double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(const double t) {
        return *this *= 1/t;
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        const auto s = 1e-8;
        return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
    }
};

// vec3 Utility Functions
inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0]+v.e[0], u.e[1]+v.e[1], u.e[2]+v.e[2]);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0]-v.e[0], u.e[1]-v.e[1], u.e[2]-v.e[2]);
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0]*v.e[0], u.e[1]*v.e[1], u.e[2]*v.e[2]);
}

inline vec3 operator*(double t, const vec3 &v) {
    return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3 &v, double t) {
    return t * v;
}

inline vec3 operator/(vec3 v, double t) {
    return (1/t) * v;
}

inline double dot(const vec3 &u, const vec3 &v) {
    return u.e[0]*v.e[0] + u.e[1]*v.e[1] + u.e[2]*v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
    return vec3(u.e[1]*v.e[2] - u.e[2]*v.e[1],
                u.e[2]*v.e[0] - u.e[0]*v.e[2],
                u.e[0]*v.e[1] - u.e[1]*v.e[0]);
}

inline vec3 unit_vector(vec3 v) {
    return v / v.length();
}

// ray Class
class ray {
public:
    vec3 orig;
    vec3 dir;
    double tm;  // time for motion blur effects

    ray() {}
    ray(const vec3& origin, const vec3& direction, double time = 0.0)
        : orig(origin), dir(direction), tm(time)
    {}

    vec3 origin() const { return orig; }
    vec3 direction() const { return dir; }
    double time() const { return tm; }

    vec3 at(double t) const {
        return orig + t * dir;
    }
};

// Forward declarations for material
class material;

// hit_record struct and hittable interface
struct hit_record {
    vec3 p;
    vec3 normal;
    shared_ptr<material> mat_ptr;
    double t;
    double u;
    double v;
    bool front_face;

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = (dot(r.direction(), outward_normal) < 0);
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const = 0;
};

// sphere Class
class sphere : public hittable {
public:
    vec3 center;
    double radius;
    shared_ptr<material> mat_ptr;

    sphere() {}
    sphere(vec3 cen, double r, shared_ptr<material> m)
        : center(cen), radius(r), mat_ptr(m)
    {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius*radius;
        auto discriminant = half_b*half_b - a*c;
        if(discriminant < 0) return false;
        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (-half_b - sqrtd) / a;
        if(root < t_min || root > t_max) {
            root = (-half_b + sqrtd) / a;
            if(root < t_min || root > t_max)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;
        // (Optional: compute uv coordinates here.)
        return true;
    }
};

// hittable_list Class
class hittable_list : public hittable {
public:
    vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = t_max;

        for(const auto& object : objects) {
            if(object->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

// Utility functions for generating random points
vec3 random_in_unit_sphere() {
    while (true) {
        vec3 p(random_double(-1,1), random_double(-1,1), random_double(-1,1));
        if (p.length_squared() >= 1) continue;
        return p;
    }
}

vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

vec3 random_in_hemisphere(const vec3& normal) {
    vec3 in_unit_sphere = random_in_unit_sphere();
    if(dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal.
        return in_unit_sphere;
    else
        return -in_unit_sphere;
}

// Reflection and Refraction Functions
vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2*dot(v, n)*n;
}

vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = min(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -std::sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

// material Interface and Derived Classes
class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {
public:
    vec3 albedo;

    lambertian(const vec3& a) : albedo(a) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        auto scatter_direction = rec.normal + random_unit_vector();
        // Catch degenerate scatter direction.
        if(scatter_direction.near_zero())
            scatter_direction = rec.normal;
        scattered = ray(rec.p, scatter_direction, r_in.time());
        attenuation = albedo;
        return true;
    }
};

class metal : public material {
public:
    vec3 albedo;
    double fuzz;

    metal(const vec3& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        auto reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(), r_in.time());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }
};

class dielectric : public material {
public:
    double ir; // Index of Refraction

    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        attenuation = vec3(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;
        vec3 unit_direction = unit_vector(r_in.direction());

        double cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if(cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction, r_in.time());
        return true;
    }
private:
    static double reflectance(double cosine, double ref_idx) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0*r0;
        return r0 + (1 - r0)*pow((1 - cosine), 5);
    }
};

// camera Class
class camera {
public:
    vec3 origin;
    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    double lens_radius;
    double time0, time1; // Shutter open/close times.

    camera(
        vec3 lookfrom,
        vec3 lookat,
        vec3 vup,
        double vfov, // vertical field-of-view in degrees.
        double aspect_ratio,
        double aperture,
        double focus_dist,
        double _time0 = 0,
        double _time1 = 0
    ) {
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);
        auto viewport_height = 2.0 * h;
        auto viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        origin = lookfrom;
        horizontal = focus_dist * viewport_width * u;
        vertical = focus_dist * viewport_height * v;
        lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist*w;

        lens_radius = aperture / 2;
        time0 = _time0;
        time1 = _time1;
    }

    ray get_ray(double s, double t) const {
        vec3 rd = lens_radius * random_in_unit_disk();
        vec3 offset = u * rd.x() + v * rd.y();
        double time = random_double(time0, time1);
        return ray(origin + offset, lower_left_corner + s*horizontal + t*vertical - origin - offset, time);
    }

private:
    vec3 random_in_unit_disk() const {
        while (true) {
            vec3 p(random_double(-1,1), random_double(-1,1), 0);
            if (p.length_squared() >= 1) continue;
            return p;
        }
    }
};

// Function to compute the color seen along a ray.
vec3 ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;
    if(depth <= 0)
        return vec3(0,0,0);

    if(world.hit(r, 0.001, infinity, rec)) {
        ray scattered;
        vec3 attenuation;
        if(rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);
        return vec3(0,0,0);
    }
    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
}

// Main Rendering Function with Multi-threading
int main() {
    // Image settings
    const auto aspect_ratio = 16.0/9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 100;
    const int max_depth = 50;

    // World: build a simple scene.
    hittable_list world;
    auto material_ground = make_shared<lambertian>(vec3(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(vec3(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.5);
    auto material_right  = make_shared<metal>(vec3(0.8, 0.6, 0.2), 0.0);

    world.add(make_shared<sphere>(vec3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(vec3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(make_shared<sphere>(vec3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(vec3(-1.0,    0.0, -1.0),  -0.45, material_left)); // Hollow sphere (glass)
    world.add(make_shared<sphere>(vec3( 1.0,    0.0, -1.0),   0.5, material_right));

    // Camera settings
    vec3 lookfrom(3,3,2);
    vec3 lookat(0,0,-1);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom - lookat).length();
    auto aperture = 2.0;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

    // Framebuffer: 2D array of colors (rows x columns)
    vector<vector<vec3>> framebuffer(image_height, vector<vec3>(image_width, vec3(0,0,0)));
    atomic<int> rows_completed(0);
    mutex mtx;

    // Render function for a block of rows.
    auto render_rows = [&](int start_row, int end_row) {
        for (int j = start_row; j < end_row; ++j) {
            for (int i = 0; i < image_width; ++i) {
                vec3 pixel_color(0,0,0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    auto u = (i + random_double()) / (image_width - 1);
                    auto v = (j + random_double()) / (image_height - 1);
                    ray r = cam.get_ray(u, v);
                    pixel_color = pixel_color + ray_color(r, world, max_depth);
                }
                framebuffer[j][i] = pixel_color;
            }
            rows_completed++;
            if (rows_completed % 10 == 0) {
                lock_guard<mutex> lock(mtx);
                cerr << "\rRows completed: " << rows_completed << "/" << image_height << std::flush;
            }
        }
    };

    // Launch multi-threaded rendering.
    int thread_count = std::thread::hardware_concurrency();
    if(thread_count == 0) thread_count = 4;
    vector<thread> threads;
    int rows_per_thread = image_height / thread_count;
    int current_row = 0;
    for (int t = 0; t < thread_count; ++t) {
        int start = current_row;
        int end = (t == thread_count - 1) ? image_height : current_row + rows_per_thread;
        threads.push_back(thread(render_rows, start, end));
        current_row = end;
    }
    for (auto& th : threads) {
        th.join();
    }

    // Write the framebuffer to a PPM image file.
    ofstream outfile("image.ppm");
    outfile << "P3\n" << image_width << " " << image_height << "\n255\n";
    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            auto color = framebuffer[j][i];
            // Divide the color by the number of samples and apply gamma correction (gamma=2.0).
            auto scale = 1.0 / samples_per_pixel;
            auto r = sqrt(scale * color.x());
            auto g = sqrt(scale * color.y());
            auto b = sqrt(scale * color.z());
            int ir = static_cast<int>(256 * clamp(r, 0.0, 0.999));
            int ig = static_cast<int>(256 * clamp(g, 0.0, 0.999));
            int ib = static_cast<int>(256 * clamp(b, 0.0, 0.999));
            outfile << ir << " " << ig << " " << ib << "\n";
        }
    }
    outfile.close();
    cerr << "\nDone.\n";
    return 0;
}
