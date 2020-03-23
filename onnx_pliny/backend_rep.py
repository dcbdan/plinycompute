from onnx.backend.base import BackendRep, namedtupledict
from .onnx_pliny import PlinyOnnx

class PlinyComputeRep(BackendRep):
  def __init__(self, model, portIn, addressIn):
    # launch PlinyCompute
    # NOTE: it is possible to have a singleton plinyOnnx object in __init__
    self.plinyOnnx = PlinyOnnx(portIn, addressIn)

    # save model...
    pass

  def __del__(self):
    # TODO: is this how plinycompute should be shutdown?
    self.plinyOnnx.shutDown()

  def run(self, inputs, **kwargs):
    """ Run PlinyComputeRep

    :param inputs: Given inputs
    :param kwargs: Other args
    :return: Outputs.
    """
    super(PlinyComputeRep, self).run(inputs, **kwargs)

    # TODO
    pass
    #return namedtupledict('Outputs', ...)(...)
