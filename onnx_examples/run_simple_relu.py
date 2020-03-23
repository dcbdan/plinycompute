import numpy as np

import onnx
from onnx_pliny import prepare

model = onnx.load("onnx_examples/simple_relu.onnx")

pliny_rep = prepare(model, 8108, "localhost")

#output = pliny_rep.run(np.random.randn(10))
#print("The classified value is", np.argmax(output))

