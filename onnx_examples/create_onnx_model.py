import onnx
import keras2onnx

from keras.models import Sequential
from keras.layers import Dense, Activation

keras_model = Sequential([
    Dense(100, input_shape=(10,)),
    Activation('relu'),
    Dense(2),
    Activation('softmax'),
])

onnx_model = keras2onnx.convert_keras(keras_model)
print("Here are the nodes of the compute graph: ")
print(onnx_model.graph.node)

print("Saving the onnx model to 'simple_relu.onnx'")
onnx.save_model(onnx_model, "simple_relu.onnx")
#
