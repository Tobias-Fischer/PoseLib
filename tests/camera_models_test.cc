#include "test.h"
#include <PoseLib/misc/camera_models.h>
#include "example_cameras.h"
#include "test.h"

using namespace poselib;

bool test_id_from_string() {

    REQUIRE_EQ(Camera::id_from_string("SIMPLE_PINHOLE"), 0);
    REQUIRE_EQ(Camera::id_from_string("PINHOLE"), 1);
    REQUIRE_EQ(Camera::id_from_string("SIMPLE_RADIAL"), 2);
    REQUIRE_EQ(Camera::id_from_string("RADIAL"), 3);
    REQUIRE_EQ(Camera::id_from_string("OPENCV"), 4);
    REQUIRE_EQ(Camera::id_from_string("OPENCV_FISHEYE"), 5);
    REQUIRE_EQ(Camera::id_from_string("FULL_OPENCV"), 6);


    return true;
}

bool test_id_string_converstion() {

    for(size_t id = 0; id < 5; ++id) {
        std::string name = Camera::name_from_id(id);
        REQUIRE_EQ(Camera::id_from_string(name), id);
    }
    return true;
}


bool test_model_name() {
    Camera camera("SIMPLE_RADIAL", {1.0, 0.0, 0.0, 0.0}, 1.0, 1.0);
    REQUIRE_EQ(camera.model_id, 2);
    REQUIRE_EQ(camera.model_name(), "SIMPLE_RADIAL");
    REQUIRE_EQ(camera.model_id, Camera::id_from_string("SIMPLE_RADIAL"));

    return true;
}


bool test_from_txt() {


    Camera camera1, camera2;

    int camera1_id = camera1.initialize_from_txt(example_camera1);    
    REQUIRE_EQ(camera1_id, 0); 
    REQUIRE_EQ(camera1.model_name(), "SIMPLE_RADIAL"); 
    REQUIRE_EQ(camera1.width, 1936);
    REQUIRE_EQ(camera1.height, 1296);
    REQUIRE_EQ(camera1.params.size(), 4);
    REQUIRE_EQ(camera1.params[0], 2425.85);
    REQUIRE_EQ(camera1.params[1],  932.383);
    REQUIRE_EQ(camera1.params[2], 628.265);
    REQUIRE_EQ(camera1.params[3], -0.0397695);
    
    int camera2_id = camera2.initialize_from_txt(example_camera2);    
    REQUIRE_EQ(camera2_id, 1); 
    REQUIRE_EQ(camera2.model_name(), "PINHOLE"); 
    REQUIRE_EQ(camera2.width, 6214);
    REQUIRE_EQ(camera2.height, 4138);
    REQUIRE_EQ(camera2.params.size(), 4);
    REQUIRE_EQ(camera2.params[0], 3425.62);
    REQUIRE_EQ(camera2.params[1], 3426.29);
    REQUIRE_EQ(camera2.params[2], 3118.41);
    REQUIRE_EQ(camera2.params[3], 2069.07);
    

    return true;
}

bool test_to_txt() {
    std::string txt0 = "SIMPLE_RADIAL 987 123 2000 1234 4567 -0.1234";
    std::string txt1 = "999 SIMPLE_RADIAL 987 123 2000 1234 4567 -0.1234";
    
    Camera camera("SIMPLE_RADIAL", {2000, 1234, 4567, -0.1234}, 987, 123);

    REQUIRE_EQ(camera.to_cameras_txt(), txt0);
    REQUIRE_EQ(camera.to_cameras_txt(999), txt1);

    // Check with example cameras
    for(size_t i = 0; i < example_cameras.size(); ++i) {
        const std::string example_txt = example_cameras[i];
        camera.initialize_from_txt(example_txt);
        REQUIRE_EQ(camera.to_cameras_txt(i), example_txt);
    }
    
    return true;
}


bool test_project_unproject() {

    for(const std::string &camera_txt : example_cameras) {
        Camera camera;
        camera.initialize_from_txt(camera_txt);
        
        for(size_t iter = 0; iter < 10; ++iter) {
            Eigen::Vector2d xp, xp2;
            Eigen::Vector3d x; 
            xp.setRandom();
            xp = 0.5 * (xp + Eigen::Vector2d(1.0, 1.0));
            xp(0) *= camera.width;
            xp(1) *= camera.height;

            camera.unproject(xp, &x);
            camera.project(x, &xp2);
            //std::cout << "xp = " << xp << " x = " << x << " xp2 = " << xp2 << "\n";
            REQUIRE((xp - xp2).norm() < 1e-6);
        }
    }
    return true;
}


void compute_jacobian_central_diff(Camera camera, Eigen::Vector3d x, Eigen::Matrix<double,2,3> &jac) {
    const double h = 1e-8;
    Eigen::Vector3d x1p(x(0) + h, x(1), x(2));
    Eigen::Vector3d x2p(x(0), x(1) + h, x(2));
    Eigen::Vector3d x3p(x(0), x(1), x(2) + h);
    
    Eigen::Vector3d x1m(x(0) - h, x(1), x(2));
    Eigen::Vector3d x2m(x(0), x(1) - h, x(2));
    Eigen::Vector3d x3m(x(0), x(1), x(2) - h);

    Eigen::Vector2d yp, ym;

    camera.project(x1p, &yp);
    camera.project(x1m, &ym);
    jac.col(0) = (yp - ym) / (2*h);

    camera.project(x2p, &yp);
    camera.project(x2m, &ym);
    jac.col(1) = (yp - ym) / (2*h);

    camera.project(x3p, &yp);
    camera.project(x3m, &ym);
    jac.col(2) = (yp - ym) / (2*h);

}


bool test_jacobian() {

    for(const std::string &camera_txt : example_cameras) {
        Camera camera;
        camera.initialize_from_txt(camera_txt);
        //std::cout << "CAMERA = " << camera.model_name() << "\n";
        for(size_t iter = 0; iter < 10; ++iter) {
            Eigen::Vector2d xp, xp2;
            Eigen::Vector3d x; 
            Eigen::Matrix<double,2,3> jac;
            xp.setRandom();
            xp = 0.5 * (xp + Eigen::Vector2d(1.0, 1.0));
            xp(0) *= 0.8 * camera.width;
            xp(1) *= 0.8 * camera.height;
            xp(0) += 0.2 * camera.width;
            xp(1) += 0.2 * camera.height;

            // Unproject
            camera.unproject(xp, &x);

            Eigen::Matrix<double,2,3> jac_finite;
            compute_jacobian_central_diff(camera, x, jac_finite);
            
            
            jac.setZero();
            camera.project_with_jac(x, &xp2, &jac);
            //std::cout << "jac = \n" << jac << "\n jac_finite = \n" << jac_finite << "\n";

            double jac_err = (jac - jac_finite).norm() / jac_finite.norm();
            //std::cout << "err = " << err <<"\n";
            REQUIRE(jac_err < 1e-6);            
            //std::cout << "point res = " << (xp - xp2).norm() << "\n";
            REQUIRE((xp - xp2).norm() < 1e-6);
        }

    }
    return true;
}
 


bool test_jacobian_1D_radial() {
    const std::string camera_txt = "0 1D_RADIAL 1920 1080 1920 1080";
    Camera camera;
    camera.initialize_from_txt(camera_txt);
    for(size_t iter = 0; iter < 10; ++iter) {
        Eigen::Vector2d xp, xp2;
        Eigen::Vector3d x; 
        Eigen::Matrix<double,2,3> jac;
        xp.setRandom();
        xp = 0.5 * (xp + Eigen::Vector2d(1.0, 1.0));
        xp(0) *= 0.8 * camera.width;
        xp(1) *= 0.8 * camera.height;
        xp(0) += 0.2 * camera.width;
        xp(1) += 0.2 * camera.height;

        // Unproject
        camera.unproject(xp, &x);
        x.normalize();

        Eigen::Matrix<double,2,3> jac_finite;
        compute_jacobian_central_diff(camera, x, jac_finite);
        
        jac.setZero();
        camera.project_with_jac(x, &xp2, &jac);
        //std::cout << "jac = \n" << jac << "\n jac_finite = \n" << jac_finite << "\n";

        double jac_err = (jac - jac_finite).norm() / jac_finite.norm();
        //std::cout << "err = " << err <<"\n";
        REQUIRE(jac_err < 1e-6);            
        //std::cout << "point res = " << (xp - xp2).norm() << "\n";
    }
    return true;
}


std::vector<Test> register_camera_models_test() {
    return {
        TEST(test_id_from_string),
        TEST(test_id_string_converstion),
        TEST(test_model_name),
        TEST(test_from_txt),
        TEST(test_to_txt),
        TEST(test_project_unproject),
        TEST(test_jacobian),
        TEST(test_jacobian_1D_radial)
    };
}