#include "varray.h"

#include <cstddef>                         // for size_t
#include <stdexcept>                       // for runtime_error
#include <type_traits>                     // for decay_t, common_type_t
#include "xtensor/xstrided_view_base.hpp"  // for strided_view_args

va::DType va::VArray::dtype() const {
    // TODO this is true now, but may not be in the future!
    return static_cast<DType>(store.index());
}

std::size_t va::VArray::size() const {
    return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());;
}

std::size_t va::VArray::dimension() const {
    return shape.size();
}

std::shared_ptr<va::VArray> va::VArray::slice(const xt::xstrided_slice_vector &slices) const {
    return std::visit([slices, this](auto &store) -> std::shared_ptr<VArray> {
        xt::detail::strided_view_args<xt::detail::no_adj_strides_policy> args;
        args.fill_args(
            shape,
            strides,
            offset,
            layout,
            slices
        );

        return std::make_shared<VArray>(VArray {
            store,  // Implicit copy
            std::move(args.new_shape),
            std::move(args.new_strides),
            args.new_offset,
            args.new_layout
        });
    }, store);
}

va::VScalar va::VArray::get_scalar(const axes_type &index) const {
    // xtensor actually checks later, too, but it just pads with 0 rather than throwing.
    if (index.size() != dimension()) throw std::runtime_error("invalid dimension for index");

    return std::visit([&index](auto&& carray) -> VScalar {
        return std::forward<decltype(carray)>(carray)[index];
    }, compute_read());
}

va::VRead va::VArray::compute_read() const {
    return std::visit([this](auto& store) -> VRead {
        using V = typename std::decay_t<decltype(store)>::element_type::value_type;
        return va::to_compute_variant<const V*>(store, *this);
    }, store);
}

va::VRead va::VArray::compute_read(const xt::xstrided_slice_vector &slices) const {
    return std::visit([this, slices](auto& store) -> VRead {
        using V = typename std::decay_t<decltype(store)>::element_type::value_type;
        return va::to_compute_variant<const V*>(store, *this, slices);
    }, store);
}

va::VWrite va::VArray::compute_write() const {
    return std::visit([this](auto& store) -> VWrite {
        using V = typename std::decay_t<decltype(store)>::element_type::value_type;
        return va::to_compute_variant<V*>(store, *this);
    }, store);
}

va::VWrite va::VArray::compute_write(const xt::xstrided_slice_vector &slices) const {
    return std::visit([this, slices](auto& store) -> VWrite {
        using V = typename std::decay_t<decltype(store)>::element_type::value_type;
        return va::to_compute_variant<V*>(store, *this, slices);
    }, store);
}

std::size_t va::VArray::size_of_array_in_bytes() const {
    return std::visit([](auto&& carray){
        using V = typename std::decay_t<decltype(carray)>::value_type;
        return carray.size() * sizeof(V);
    }, compute_read());
}

va::VScalar va::dtype_to_variant(const DType dtype) {
    switch (dtype) {
        case DType::Bool:
            return bool();
        case DType::Float32:
            return float_t();
        case DType::Float64:
            return double_t();
        case DType::Int8:
            return int8_t();
        case DType::Int16:
            return int16_t();
        case DType::Int32:
            return int32_t();
        case DType::Int64:
            return int64_t();
        case DType::UInt8:
            return uint8_t();
        case DType::UInt16:
            return uint16_t();
        case DType::UInt32:
            return uint32_t();
        case DType::UInt64:
            return int64_t();
        default:
            throw std::runtime_error("Invalid dtype.");
    }
}

va::DType va::variant_to_dtype(const VScalar dtype) {
    return static_cast<DType>(dtype.index());
}

std::size_t va::size_of_dtype_in_bytes(const DType dtype) {
    return std::visit([](auto dtype){
        return sizeof(dtype);
    }, dtype_to_variant(dtype));
}

va::VScalar va::scalar_to_dtype(const VScalar v, const DType dtype) {
    return std::visit([](auto v, const auto t) -> va::VScalar {
        using T = std::decay_t<decltype(t)>;
        return static_cast<T>(v);
    }, v, dtype_to_variant(dtype));
}

va::DType va::dtype_common_type(const DType a, const DType b) {
    if (a == DTypeMax) return b;
    if (b == DTypeMax) return a;

    return std::visit(
        [](auto a, auto b) { return variant_to_dtype(std::common_type_t<decltype(a), decltype(b)>()); },
        dtype_to_variant(a),
        dtype_to_variant(b)
    );
}

va::VScalar va::VArray::to_single_value() const {
    return std::visit([](const auto& carray) -> va::VScalar {
        if (carray.size() != 1) {
            throw std::runtime_error("Expected a single element after slicing.");
        }
        return *carray.data();
        // TODO I expected this to work, but it doesn't. See https://xtensor.readthedocs.io/en/latest/indices.html#operator
        // But at least the above is a view, so no copy is made.
        // return V(array[slice]);
    }, compute_read());
}

va::VArray::operator bool() const { return va::scalar_to_type<bool>(to_single_value()); }
va::VArray::operator int64_t() const { return va::scalar_to_type<int64_t>(to_single_value()); }
va::VArray::operator int32_t() const { return va::scalar_to_type<int32_t>(to_single_value()); }
va::VArray::operator int16_t() const { return va::scalar_to_type<int16_t>(to_single_value()); }
va::VArray::operator int8_t() const { return va::scalar_to_type<int8_t>(to_single_value()); }
va::VArray::operator uint64_t() const { return va::scalar_to_type<uint64_t>(to_single_value()); }
va::VArray::operator uint32_t() const { return va::scalar_to_type<uint32_t>(to_single_value()); }
va::VArray::operator uint16_t() const { return va::scalar_to_type<uint16_t>(to_single_value()); }
va::VArray::operator uint8_t() const { return va::scalar_to_type<uint8_t>(to_single_value()); }
va::VArray::operator double() const { return va::scalar_to_type<double>(to_single_value()); }
va::VArray::operator float() const { return va::scalar_to_type<float>(to_single_value()); }

std::shared_ptr<va::VArray> va::from_scalar_variant(VScalar scalar) {
    return std::visit([](auto cscalar){
        return from_scalar(cscalar);
    }, scalar);
}
