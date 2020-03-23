#ifndef ONNX_PLINY_CC
#define ONNX_PLINY_CC

#include <boost/python.hpp>
#include <PDBOnnx.h>

BOOST_PYTHON_MODULE(onnx_pliny)
{
  using namespace boost::python;
  using namespace pdb;

  // all python interface goes by 'Pliny', not by 'PDB'
  class_<PDBOnnx>("PlinyOnnx", init<int, std::string>())
      .def("helloWorld", &PDBOnnx::helloWorld)
      .def("doSomething", &PDBOnnx::doSomething)
      .def("shutDown", &PDBOnnx::shutDown);
}

#endif
