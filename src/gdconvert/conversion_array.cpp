#include "conversion_array.h"

#include <vatensor/allocate.h>                         // for empty
#include <cmath>                                       // for double_t, float_t
#include <cstddef>                                     // for size_t
#include <cstdint>                                     // for int32_t, int64_t
#include <memory>                                      // for allocator, sha...
#include <stdexcept>                                   // for runtime_error
#include <tuple>                                       // for tuple
#include <utility>                                     // for move
#include <vector>                                      // for vector
#include "godot_cpp/classes/object.hpp"                // for Object
#include "godot_cpp/core/defs.hpp"                     // for real_t
#include "godot_cpp/core/object.hpp"                   // for Object::cast_to
#include "godot_cpp/variant/packed_byte_array.hpp"     // for PackedByteArray
#include "godot_cpp/variant/packed_float32_array.hpp"  // for PackedFloat32A...
#include "godot_cpp/variant/packed_float64_array.hpp"  // for PackedFloat64A...
#include "godot_cpp/variant/packed_int32_array.hpp"    // for PackedInt32Array
#include "godot_cpp/variant/packed_int64_array.hpp"    // for PackedInt64Array
#include "godot_cpp/variant/variant.hpp"               // for Variant
#include "godot_cpp/variant/vector2.hpp"               // for Vector2
#include "godot_cpp/variant/vector2i.hpp"              // for Vector2i
#include "godot_cpp/variant/vector3.hpp"               // for Vector3
#include "godot_cpp/variant/vector3i.hpp"              // for Vector3i
#include "godot_cpp/variant/vector4.hpp"               // for Vector4
#include "godot_cpp/variant/vector4i.hpp"              // for Vector4i
#include "ndarray.h"                                   // for NDArray
#include "xtensor/xadapt.hpp"                          // for adapt
#include "xtensor/xarray.hpp"                          // for xarray_container
#include "xtensor/xbuffer_adaptor.hpp"                 // for no_ownership
#include "xtensor/xlayout.hpp"                         // for layout_type
#include "xtensor/xshape.hpp"                          // for static_shape
#include "xtensor/xstorage.hpp"                        // for svector, uvector
#include "xtensor/xstrided_view.hpp"                   // for xstrided_slice...
#include "xtensor/xtensor_forward.hpp"                 // for xarray
#include "xtl/xiterator_base.hpp"                      // for operator+

template <typename C, typename T>
va::VArray packed_as_xarray(const T shape_array) {
    auto size = static_cast<std::size_t>(shape_array.size());

    xt::static_shape<std::size_t, 1> shape_of_shape = { size };

    auto store = std::make_shared<xt::xarray<C>>(
        xt::xarray<C>(xt::adapt(shape_array.ptr(), size, xt::no_ownership(), shape_of_shape))
    );

    return va::from_store(store);
}

void add_size_at_idx(va::shape_type& shape, const std::size_t idx, const std::size_t value) {
    if (shape.size() > idx) {
        const auto current_dim_size = shape[idx];

        // Sizes are the same.
        if (current_dim_size == value) return;

        // We defined the size before, this element can broadcast.
        if (value == 1 && current_dim_size >= 1) return;

        // We broadcasted before, this element defines the size.
        if (current_dim_size == 1 && value > 1) {
            shape[idx] = value;
            return;
        }

        throw std::runtime_error("array has an inhomogenous shape");
    }
    else if (shape.size() == idx) {
        shape.push_back(value);
    }
    else {
        throw std::invalid_argument("index out of range");
    }
}

void find_shape_and_dtype(va::shape_type& shape, va::DType &dtype, const Array& input_array) {
    std::vector<Array> current_dim_arrays = { input_array };

    for (size_t current_dim_idx = 0; true; ++current_dim_idx) {
        for (const auto& array : current_dim_arrays) {
            add_size_at_idx(shape, current_dim_idx, array.size());
        }

        std::vector<Array> next_dim_arrays;
        for (const Array& array : current_dim_arrays) {
            for (int i = 0; i < array.size(); ++i) {
                const Variant& array_element = array[i];

                switch (array_element.get_type()) {
                    case Variant::OBJECT: {
                        if (const auto ndarray = Object::cast_to<NDArray>(array_element)) {
                            auto varray_dim_idx = current_dim_idx;
                            for (const auto size : ndarray->array.shape) {
                                add_size_at_idx(shape, varray_dim_idx, size);
                                varray_dim_idx++;
                            }
                            continue;
                        }
                    }
                    case Variant::ARRAY:
                        next_dim_arrays.push_back(array_element);
                        continue;
                    case Variant::BOOL:
                        dtype = va::dtype_common_type(dtype, va::Bool);
                        continue;
                    case Variant::INT:
                        dtype = va::dtype_common_type(dtype, va::Int64);
                        continue;
                    case Variant::FLOAT:
                        dtype = va::dtype_common_type(dtype, va::Float64);
                        continue;
                    case Variant::PACKED_BYTE_ARRAY: {
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(uint8_t()));
                        PackedByteArray packed = array_element;
                        add_size_at_idx(shape, current_dim_idx + 1, packed.size());
                        continue;
                    }
                    case Variant::PACKED_INT32_ARRAY: {
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(int32_t()));
                        PackedInt32Array packed = array_element;
                        add_size_at_idx(shape, current_dim_idx + 1, packed.size());
                        continue;
                    }
                    case Variant::PACKED_INT64_ARRAY: {
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(int64_t()));
                        PackedInt64Array packed = array_element;
                        add_size_at_idx(shape, current_dim_idx + 1, packed.size());
                        continue;
                    }
                    case Variant::PACKED_FLOAT32_ARRAY: {
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(float_t()));
                        PackedFloat32Array packed = array_element;
                        add_size_at_idx(shape, current_dim_idx + 1, packed.size());
                        continue;
                    }
                    case Variant::PACKED_FLOAT64_ARRAY: {
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(double_t()));
                        PackedFloat64Array packed = array_element;
                        add_size_at_idx(shape, current_dim_idx + 1, packed.size());
                        continue;
                    }
                    case Variant::VECTOR2I:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(int64_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 2);
                        continue;
                    case Variant::VECTOR3I:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(int64_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 3);
                        continue;
                    case Variant::VECTOR4I:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(int64_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 4);
                        continue;
                    case Variant::VECTOR2:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(real_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 2);
                        continue;
                    case Variant::VECTOR3:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(real_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 3);
                        continue;
                    case Variant::VECTOR4:
                        dtype = va::dtype_common_type(dtype, va::variant_to_dtype(real_t()));
                        add_size_at_idx(shape, current_dim_idx + 1, 4);
                        continue;
                    default:
                        break;
                }

                throw std::runtime_error("unsupported array type");
            }
        }

        if (next_dim_arrays.empty()) break;

        current_dim_arrays = std::move(next_dim_arrays);
    }
}

va::VArray array_as_varray(const Array& input_array) {
    va::shape_type shape;
    va::DType dtype = va::DTypeMax;

    find_shape_and_dtype(shape, dtype, input_array);

    if (dtype == va::DTypeMax) dtype = va::Float64; // Default dtype

    va::VArray varray = va::empty(dtype, shape);
    std::vector<std::tuple<xt::xstrided_slice_vector, Array>> next = { std::make_tuple(xt::xstrided_slice_vector {}, input_array) };

    while (!next.empty()) {
        const auto [array_base_idx, array] = std::move(next.back());
        next.pop_back();

        for (size_t i = 0; i < array.size(); ++i) {
            auto element_idx = array_base_idx;
            element_idx.emplace_back(i);

            const auto& array_element = array[i];
            switch (array_element.get_type()) {
                case Variant::OBJECT: {
                    if (const auto ndarray = Object::cast_to<NDArray>(array_element)) {
                        varray.slice(element_idx).set_with_array(ndarray->array);
                        continue;
                    }
                }
                case Variant::ARRAY:
                    next.emplace_back(element_idx, static_cast<Array>(array_element));
                    continue;
                case Variant::BOOL:
                    // TODO If we're on the last dimension, we should use element assign rather than slice - fill for all these.
                    varray.slice(element_idx).fill(static_cast<bool>(array_element));
                    continue;
                case Variant::INT:
                    varray.slice(element_idx).fill(static_cast<int64_t>(array_element));
                    continue;
                case Variant::FLOAT:
                    varray.slice(element_idx).fill(static_cast<double_t>(array_element));
                    continue;
                case Variant::PACKED_BYTE_ARRAY: {
                    varray.slice(element_idx).set_with_array(packed_as_xarray<uint8_t>(PackedByteArray(array_element)));
                    continue;
                }
                case Variant::PACKED_INT32_ARRAY: {
                    varray.slice(element_idx).set_with_array(packed_as_xarray<uint8_t>(PackedInt32Array(array_element)));
                    continue;
                }
                case Variant::PACKED_INT64_ARRAY: {
                    varray.slice(element_idx).set_with_array(packed_as_xarray<uint8_t>(PackedInt64Array(array_element)));
                    continue;
                }
                case Variant::PACKED_FLOAT32_ARRAY: {
                    varray.slice(element_idx).set_with_array(packed_as_xarray<uint8_t>(PackedFloat32Array(array_element)));
                    continue;
                }
                case Variant::PACKED_FLOAT64_ARRAY: {
                    varray.slice(element_idx).set_with_array(packed_as_xarray<uint8_t>(PackedFloat64Array(array_element)));
                    continue;
                }
                case Variant::VECTOR2I: {
                    const Vector2i vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    continue;
                }
                case Variant::VECTOR3I: {
                    const Vector3i vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    element_idx.back() = 2;
                    varray.slice(element_idx).fill(vector.z);
                    continue;
                }
                case Variant::VECTOR4I: {
                    const Vector4i vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    element_idx.back() = 2;
                    varray.slice(element_idx).fill(vector.z);
                    element_idx.back() = 3;
                    varray.slice(element_idx).fill(vector.w);
                    continue;
                }
                case Variant::VECTOR2: {
                    const Vector2 vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    continue;
                }
                case Variant::VECTOR3: {
                    const Vector3 vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    element_idx.back() = 2;
                    varray.slice(element_idx).fill(vector.z);
                    continue;
                }
                case Variant::VECTOR4: {
                    const Vector4 vector = array_element;
                    element_idx.emplace_back(0);
                    varray.slice(element_idx).fill(vector.x);
                    element_idx.back() = 1;
                    varray.slice(element_idx).fill(vector.y);
                    element_idx.back() = 2;
                    varray.slice(element_idx).fill(vector.z);
                    element_idx.back() = 3;
                    varray.slice(element_idx).fill(vector.w);
                    continue;
                }
                default:
                    break;
            }

            throw std::runtime_error("unsupported array type");
        }
    }

    return varray;
}

va::VArray variant_as_array(const Variant& array) {
    switch (array.get_type()) {
        case Variant::OBJECT: {
            if (const auto ndarray = Object::cast_to<NDArray>(array)) {
                return ndarray->array;
            }
            break;
        }
        case Variant::ARRAY: {
            return array_as_varray(array);
        }
        case Variant::BOOL: {
            return va::from_store(std::make_shared<xt::xarray<bool>>(xt::xarray<bool>(array)));
        }
        case Variant::INT: {
            return va::from_store(std::make_shared<xt::xarray<int64_t>>(xt::xarray<int64_t>(array)));
        }
        case Variant::FLOAT: {
            return va::from_store(std::make_shared<xt::xarray<double_t>>(xt::xarray<double_t>(array)));
        }
        case Variant::PACKED_BYTE_ARRAY:
            return packed_as_xarray<uint8_t>(PackedByteArray(array));
        case Variant::PACKED_INT32_ARRAY:
            return packed_as_xarray<int32_t>(PackedInt32Array(array));
        case Variant::PACKED_INT64_ARRAY:
            return packed_as_xarray<int64_t>(PackedInt64Array(array));
        case Variant::PACKED_FLOAT32_ARRAY:
            return packed_as_xarray<float_t>(PackedFloat32Array(array));
        case Variant::PACKED_FLOAT64_ARRAY:
            return packed_as_xarray<double_t>(PackedFloat64Array(array));
        case Variant::VECTOR2I: {
            auto vector = Vector2i(array);
            return va::from_store(std::make_shared<xt::xarray<int32_t>>(xt::xarray<int32_t>(
                { vector.x, vector.y }
            )));
        }
        case Variant::VECTOR3I: {
            auto vector = Vector3i(array);
            return va::from_store(std::make_shared<xt::xarray<int32_t>>(xt::xarray<int32_t>(
                { vector.x, vector.y, vector.z }
            )));
        }
        case Variant::VECTOR4I: {
            auto vector = Vector4i(array);
            return va::from_store(std::make_shared<xt::xarray<int32_t>>(xt::xarray<int32_t>(
                { vector.x, vector.y, vector.z, vector.w }
            )));
        }
        case Variant::VECTOR2: {
            auto vector = Vector2(array);
            return va::from_store(std::make_shared<xt::xarray<real_t>>(xt::xarray<real_t>(
                { vector.x, vector.y }
            )));
        }
        case Variant::VECTOR3: {
            auto vector = Vector3(array);
            return va::from_store(std::make_shared<xt::xarray<real_t>>(xt::xarray<real_t>(
                { vector.x, vector.y, vector.z }
            )));
        }
        case Variant::VECTOR4: {
            auto vector = Vector4(array);
            return va::from_store(std::make_shared<xt::xarray<real_t>>(xt::xarray<real_t>(
                { vector.x, vector.y, vector.z, vector.w }
            )));
        }
        default:
            break;
    }

    throw std::runtime_error("Unsupported type");
}

Array varray_to_godot_array(const va::VArray& array) {
#ifdef NUMDOT_DISABLE_GODOT_CONVERSION_FUNCTIONS
    throw std::runtime_error("function explicitly disabled; recompile without NUMDOT_DISABLE_GODOT_CONVERSION_FUNCTIONS to enable it.");
#else
    auto godot_array = Array();

    // TODO Non-flat
    std::visit([&godot_array](auto carray){
        godot_array.resize(carray.size());
        auto start = carray.begin();

        for (int64_t i = 0; i < carray.size(); ++i) {
            godot_array[i] = *(start + i);
        }
    }, array.to_compute_variant());

    return godot_array;
#endif
}
