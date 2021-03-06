// Copyright (c) 2015-2015 Rinat Zakirov
// Copyright (c) 2015-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Remote.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/comms/tests", test_symbol_byte_conversions)
{
    //run the topology
    for (int mod = 1; mod <= 8; mod++)
    for (int i = 0; i < 2; i++)
    {
        const std::string order = i == 0 ? "LSBit" : "MSBit";
        std::cout << "run the topology with " << order << " order ";
        std::cout << "and " << mod << " modulus" << std::endl;

        auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");

        //path 0 tests symbols to bytes -> bytes to symbols
        auto symsToBytes0 = Pothos::BlockRegistry::make("/comms/symbols_to_bytes");
        symsToBytes0.call("setModulus", mod);
        symsToBytes0.call("setBitOrder", order);
        auto bytesToSyms0 = Pothos::BlockRegistry::make("/comms/bytes_to_symbols");
        bytesToSyms0.call("setModulus", mod);
        bytesToSyms0.call("setBitOrder", order);
        auto collector0 = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

        //path 1 tests symbols to bytes -> symbols to bits (8) -> bits to symbols
        auto symsToBytes1 = Pothos::BlockRegistry::make("/comms/symbols_to_bytes");
        symsToBytes1.call("setModulus", mod);
        symsToBytes1.call("setBitOrder", order);
        auto symsToBits1 = Pothos::BlockRegistry::make("/comms/symbols_to_bits");
        symsToBits1.call("setModulus", 8);
        symsToBits1.call("setBitOrder", order);
        auto bitsToSyms1 = Pothos::BlockRegistry::make("/comms/bits_to_symbols");
        bitsToSyms1.call("setModulus", mod);
        bitsToSyms1.call("setBitOrder", order);
        auto collector1 = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

        //path 2 tests symbols to bits -> bits to symbols (8) -> bytes to symbols
        auto symsToBits2 = Pothos::BlockRegistry::make("/comms/symbols_to_bits");
        symsToBits2.call("setModulus", mod);
        symsToBits2.call("setBitOrder", order);
        auto bitsToSyms2 = Pothos::BlockRegistry::make("/comms/bits_to_symbols");
        bitsToSyms2.call("setModulus", 8);
        bitsToSyms2.call("setBitOrder", order);
        auto bytesToSyms2 = Pothos::BlockRegistry::make("/comms/bytes_to_symbols");
        bytesToSyms2.call("setModulus", mod);
        bytesToSyms2.call("setBitOrder", order);
        auto collector2 = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

        Pothos::Topology topology;

        //setup path 0
        topology.connect(feeder, 0, symsToBytes0, 0);
        topology.connect(symsToBytes0, 0, bytesToSyms0, 0);
        topology.connect(bytesToSyms0, 0, collector0, 0);

        //setup path 1
        topology.connect(feeder, 0, symsToBytes1, 0);
        topology.connect(symsToBytes1, 0, symsToBits1, 0);
        topology.connect(symsToBits1, 0, bitsToSyms1, 0);
        topology.connect(bitsToSyms1, 0, collector1, 0);

        //setup path 2
        topology.connect(feeder, 0, symsToBits2, 0);
        topology.connect(symsToBits2, 0, bitsToSyms2, 0);
        topology.connect(bitsToSyms2, 0, bytesToSyms2, 0);
        topology.connect(bytesToSyms2, 0, collector2, 0);

        //create a test plan for streams
        //total multiple required to flush out complete stream
        std::cout << "Perform stream-based test plan..." << std::endl;
        {
            json testPlan;
            testPlan["enableBuffers"] = true;
            testPlan["totalMultiple"] = 8;
            testPlan["minValue"] = 0;
            testPlan["maxValue"] = (1 << mod) - 1;
            auto expected = feeder.call("feedTestPlan", testPlan.dump());
            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.01));

            std::cout << "verifyTestPlan path0..." << std::endl;
            collector0.call("verifyTestPlan", expected);
            std::cout << "verifyTestPlan path1..." << std::endl;
            collector1.call("verifyTestPlan", expected);
            std::cout << "verifyTestPlan path2..." << std::endl;
            collector2.call("verifyTestPlan", expected);
        }

        //create a test plan for packets
        //buffer multiple required to avoid padding packets in loopback test
        std::cout << "Perform packet-based test plan..." << std::endl;
        {
            json testPlan;
            testPlan["enablePackets"] = true;
            testPlan["bufferMultiple"] = 8;
            testPlan["minValue"] = 0;
            testPlan["maxValue"] = (1 << mod) - 1;
            auto expected = feeder.call("feedTestPlan", testPlan.dump());
            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.01));

            std::cout << "verifyTestPlan path0..." << std::endl;
            collector0.call("verifyTestPlan", expected);
            std::cout << "verifyTestPlan path1..." << std::endl;
            collector1.call("verifyTestPlan", expected);
            std::cout << "verifyTestPlan path2..." << std::endl;
            collector2.call("verifyTestPlan", expected);
        }
    }

    std::cout << "done!\n";
}
