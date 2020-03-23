from onnx.backend.base import Backend

from .backend_rep import PlinyComputeRep

class PlinyComputeBackend(Backend):
  """ PlinyCompute Backend for ONNX
  """

  @classmethod
  def prepare(cls,
              model,
              portIn,
              addressIn,
              device='CPU',
              **kwargs):
    """Prepare an ONNX model for PlinyCompute Backend.

    Launch Pliny Compute and construct the model.

    :param model: The ONNX model to construct in PlinyCompute.
    :param device: device is currently not passed to PlinyCompute in any way

    Returns:
      A PlinyComputeRep class object representing the ONNX model
    """
    super(PlinyComputeBackend, cls).prepare(model, device, **kwargs)

    # TODO: what should be done with device? See onnxruntime implementation. They have a
    #       capi.

    # TODO: decide whether or not to pass in logging_level='INFO' and make use of python's
    #       logging package. See onnx-tensorflow implementation.

    return PlinyComputeRep(model, portIn, addressIn, **kwargs)

  def run_node(cls,
               node,
               inputs,
               device='CPU',
               outputs_info=None,
               **kwargs):
    """ This method is not implemented as it is much more efficient
    to run a while model than every node independently.
    """
    raise NotImplementedError("It is more efficient to run the whole model than every node independently.")

prepare = PlinyComputeBackend.prepare

run_model = PlinyComputeBackend.run_model



