/*
 * Copyright 2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "delay.hpp"

class delay : public adf::graph {
   public:
    // input and output port
    adf::input_plio image_points_from_PL;
    adf::input_plio image_points_from_PL_;
    adf::input_plio tx_def_ref_point;
    adf::input_plio tx_def_delay_distance;
    adf::input_plio tx_def_delay_distance2;
    adf::input_plio tx_def_focal_point;
    adf::input_plio t_start;
    adf::output_plio delay_to_PL;

    // L2 graph
    us::L2::delay_graph<> d;

    delay() {
        // input & output plio
        image_points_from_PL =
            adf::input_plio::create("image_points_from_PL", adf::plio_32_bits, "data/image_points.txt");
        image_points_from_PL_ =
            adf::input_plio::create("image_points_from_PL2", adf::plio_32_bits, "data/image_points.txt");
        tx_def_ref_point = adf::input_plio::create("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_delay_distance =
            adf::input_plio::create("tx_def_delay_distance", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_delay_distance2 =
            adf::input_plio::create("tx_def_delay_distance2", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_focal_point =
            adf::input_plio::create("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        t_start = adf::input_plio::create("t_start", adf::plio_32_bits, "data/t_start.txt");
        delay_to_PL = adf::output_plio::create("delay_to_PL", adf::plio_32_bits, "data/delay_to_PL.txt");

        // connections
        adf::connect<>(image_points_from_PL.out[0], d.image_points_from_PL);
        adf::connect<>(image_points_from_PL_.out[0], d.image_points_from_PL_);
        adf::connect<>(tx_def_ref_point.out[0], d.tx_def_ref_point);
        adf::connect<>(tx_def_delay_distance.out[0], d.tx_def_delay_distance);
        adf::connect<>(tx_def_delay_distance2.out[0], d.tx_def_delay_distance2);
        adf::connect<>(tx_def_focal_point.out[0], d.tx_def_focal_point);
        adf::connect<>(t_start.out[0], d.t_start);
        adf::connect<>(d.delay_to_PL, delay_to_PL.in[0]);
    }
};

delay b;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif