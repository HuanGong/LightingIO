#include "example_app.h"
#include "gflags/gflags.h"

int main(int argc, char** argv) {

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  content::GeneralServerApp  app;

  app.RunApplication();
}
