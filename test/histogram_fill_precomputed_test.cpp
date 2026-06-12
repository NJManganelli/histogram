// Copyright 2026 Hans Dembinski, Nick Manganelli
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/histogram/accumulators/weighted_sum.hpp>
#include <boost/histogram/algorithm/sum.hpp>
#include <boost/histogram/axis/category.hpp>
#include <boost/histogram/axis/integer.hpp>
#include <boost/histogram/axis/ostream.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/fill_n.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/literals.hpp>
#include <boost/histogram/make_histogram.hpp>
#include <boost/histogram/ostream.hpp>
#include <boost/variant2/variant.hpp>
#include <string>
#include <vector>
#include "histogram.hpp"
#include "throw_exception.hpp"

using namespace boost::histogram;
using namespace boost::histogram::algorithm;
using boost::variant2::variant;

using precomputed = detail::precomputed_bin_indices;

using in = axis::integer<int, axis::null_type>;          // underflow|overflow
using cs = axis::category<std::string, axis::null_type>; // overflow
using cs0 = axis::category<std::string, axis::null_type, axis::option::none_t>;

template <class Tag>
void run_tests() {
  // 1D: fill with precomputed indices is equivalent to fill with values
  {
    auto h = make(Tag(), cs{"A", "B"});
    auto h2 = h;

    const std::vector<std::string> data = {"A", "B", "A", "C"}; // C -> overflow
    for (auto&& s : data) h(s);

    // the indices as axis::traits::index returns them: A=0, B=1, C=2 (overflow)
    const std::vector<axis::index_type> idx = {0, 1, 0, 2};
    using V = variant<std::vector<std::string>, precomputed>;
    V v[1];
    v[0] = precomputed(idx.data(), idx.size());
    h2.fill(v);

    BOOST_TEST_EQ(h, h2);
    BOOST_TEST_EQ(sum(h2), 4);
  }

  // 1D without overflow: out-of-range index invalidates the entry,
  // like an out-of-range value does
  {
    auto h = make(Tag(), cs0{"A", "B"});
    auto h2 = h;

    const std::vector<std::string> data = {"A", "B", "C"}; // C is dropped
    for (auto&& s : data) h(s);

    const std::vector<axis::index_type> idx = {0, 1, 2}; // 2 is out of range
    using V = variant<std::vector<std::string>, precomputed>;
    V v[1];
    v[0] = precomputed(idx.data(), idx.size());
    h2.fill(v);

    BOOST_TEST_EQ(h, h2);
    BOOST_TEST_EQ(sum(h2), 2);
  }

  // 2D: values for one axis, precomputed indices for the other, with weights;
  // the value axis has underflow and overflow and receives flow values
  {
    auto h = make_s(Tag(), weight_storage(), in{0, 3}, cs{"A", "B"});
    auto h2 = h;

    const std::vector<int> x = {-1, 0, 1, 2, 3}; // -1 underflow, 3 overflow
    const std::vector<std::string> s = {"A", "B", "B", "A", "C"};
    const std::vector<double> w = {1, 2, 3, 4, 5};
    for (unsigned i = 0; i < x.size(); ++i) h(x[i], s[i], weight(w[i]));

    const std::vector<axis::index_type> idx = {0, 1, 1, 0, 2};
    using V = variant<int, std::vector<int>, precomputed>;
    V v[2];
    v[0] = x;
    v[1] = precomputed(idx.data(), idx.size());
    h2.fill(v, weight(w));

    BOOST_TEST_EQ(h, h2);
  }

  // 2D: scalar broadcast for one axis, precomputed indices for the other
  {
    auto h = make(Tag(), in{0, 3}, cs{"A", "B"});
    auto h2 = h;

    const std::vector<std::string> s = {"A", "B", "B"};
    for (auto&& si : s) h(1, si);

    const std::vector<axis::index_type> idx = {0, 1, 1};
    using V = variant<int, std::vector<int>, precomputed>;
    V v[2];
    v[0] = 1;
    v[1] = precomputed(idx.data(), idx.size());
    h2.fill(v);

    BOOST_TEST_EQ(h, h2);
  }

  // 2D: precomputed indices for both axes
  {
    auto h = make(Tag(), cs{"A", "B"}, cs{"X", "Y"});
    auto h2 = h;

    const std::vector<std::string> s1 = {"A", "B", "B"};
    const std::vector<std::string> s2 = {"Y", "X", "Y"};
    for (unsigned i = 0; i < s1.size(); ++i) h(s1[i], s2[i]);

    const std::vector<axis::index_type> idx1 = {0, 1, 1};
    const std::vector<axis::index_type> idx2 = {1, 0, 1};
    using V = variant<std::vector<std::string>, precomputed>;
    V v[2];
    v[0] = precomputed(idx1.data(), idx1.size());
    v[1] = precomputed(idx2.data(), idx2.size());
    h2.fill(v);

    BOOST_TEST_EQ(h, h2);
  }

  // length mismatch throws
  {
    auto h = make(Tag(), in{0, 3}, cs{"A", "B"});

    const std::vector<int> x = {0, 1, 2};
    const std::vector<axis::index_type> idx = {0, 1}; // wrong length
    using V = variant<int, std::vector<int>, precomputed>;
    V v[2];
    v[0] = x;
    v[1] = precomputed(idx.data(), idx.size());
    BOOST_TEST_THROWS(h.fill(v), std::invalid_argument);
  }
}

int main() {
  run_tests<static_tag>();
  run_tests<dynamic_tag>();

  return boost::report_errors();
}
