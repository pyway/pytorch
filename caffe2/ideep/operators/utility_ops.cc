#include "caffe2/operators/utility_ops.h"
#include "caffe2/core/operator.h"
#include "caffe2/ideep/ideep_utils.h"

namespace caffe2 {

class CopyCPUToIDEEPOp final : public IDEEPOperator {
 public:
  USE_SIMPLE_IDEEP_CTOR_DTOR(CopyCPUToIDEEPOp);
  USE_IDEEP_DEF_ALIASES();

  bool RunOnDevice() override {
    const auto& X = OperatorBase::Input<Tensor>(0, CPU);
    auto* Y = OperatorBase::OutputBlob(0);
    itensor::dims src_dims(X.dims().begin(), X.dims().end());
    if (!(Y->template IsType<itensor>() &&
          Y->Get<itensor>().get_data_type() == itensor::data_type::f32) ||
        Y->Get<itensor>().get_dims() != src_dims) {
      Y->Reset(new itensor());
      Y->GetMutable<itensor>()->resize(src_dims, itensor::data_type::f32);
    }
    Y->GetMutable<itensor>()->reorder_from(
        src_dims, itensor::data_type::f32, X.raw_data());
    return true;
  }
};

class CopyIDEEPToCPUOp final : public IDEEPOperator {
 public:
  USE_SIMPLE_IDEEP_CTOR_DTOR(CopyIDEEPToCPUOp);
  USE_IDEEP_DEF_ALIASES();
  bool RunOnDevice() override {
    const auto& input_blob = OperatorBase::InputBlob(0);
    if (BlobIsTensorType(input_blob, CPU)) {
      VLOG(2) << "Directing sharing of TensorCPU";
      const auto& X = OperatorBase::Input<Tensor>(0, CPU);
      auto* Y = OperatorBase::Output<Tensor>(0, CPU);
      Y->CopyFrom(X);
    } else {
      const auto& X = OperatorBase::Input<itensor>(0);
      auto* Y = OperatorBase::Output<Tensor>(0, CPU);
      Y->Resize(X.get_dims());
      if (X.get_data_type() == itensor::data_type::f32) {
        X.reorder_to(Y->template mutable_data<float>());
      } else {
        CAFFE_THROW("Unsupported ideep type: ", X.get_data_type());
      }
    }
    return true;
  }
};

REGISTER_IDEEP_OPERATOR(CopyCPUToIDEEP, CopyCPUToIDEEPOp);
REGISTER_IDEEP_OPERATOR(CopyIDEEPToCPU, CopyIDEEPToCPUOp);

OPERATOR_SCHEMA(CopyCPUToIDEEP)
    .NumInputs(1)
    .NumOutputs(1)
    .Input(0, "cpu_blob", "The input TensorCPU to copy")
    .Output(0, "ideep_blob", "The output IDEEP tensort to copy to");
OPERATOR_SCHEMA(CopyIDEEPToCPU)
    .NumInputs(1)
    .NumOutputs(1)
    .Input(0, "ideep_blob", "The input IDEEP tensort to copy")
    .Output(0, "cpu_blob", "The output TensorCPU to copy to");

} // namespace caffe2
