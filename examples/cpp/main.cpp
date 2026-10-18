// main.cpp. How to use protacoc generated code.
// Steps: include the generated header, set values and call Serialize(), receiver calls protaco::ToObject<T>(bytes).
#include <cstdio>
#include <string>
#include <vector>

#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"
#include "TransferRequest.g.h"

namespace
{

// 1. Basics: send and receive a message with lists and nested messages.
void SendAndReceive()
{
    demo::TransferRequest request;
    request.set_tm("TM1");
    demo::MoveInfo move;
    move.set_from("SHELF1");
    move.set_to("TABLE3");
    move.set_slot(2);
    request.get_mutable_moves().push_back(move);
    request.set_reason("closest-first");

    const std::vector<uint8_t>& bytes = request.Serialize();   // send bytes.data(), bytes.size()
    std::printf("TransferRequest: %zu bytes\n", bytes.size());

    const demo::TransferRequest got = protaco::ToObject<demo::TransferRequest>(bytes);
    std::printf("  tm=%s first=%s->%s slot=%d reason=%s\n",
        got.get_tm().c_str(), got.get_moves()[0].get_from().c_str(), got.get_moves()[0].get_to().c_str(),
        got.get_moves()[0].get_slot(), got.get_reason().c_str());
}

// 2. Fixed fields only: setters already serialize, Serialize() returns the buffer as is.
void FixedOnly()
{
    demo::Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_uptimeSec(10);
    const std::vector<uint8_t>& bytes = beat.Serialize();
    beat.set_uptimeSec(11);   // written into the same buffer, no need to call Serialize again
    std::printf("Heartbeat: received uptime=%d, same buffer=%s\n",
        protaco::ToObject<demo::Heartbeat>(bytes).get_uptimeSec(), &bytes == &beat.Serialize() ? "yes" : "no");
}

// 3. Extra data without touching the yaml (ext). Receivers keep keys they do not know.
void ExtraData()
{
    demo::PizzaOrder order;
    order.set_orderId("A-100");
    demo::Topping basil;
    basil.set_name("basil");
    order.get_mutable_toppings().push_back(basil);
    order.get_mutable_ext().SetString("secretSauce", "yes please");   // key not in the yaml
    order.get_mutable_ext().SetInt("pineapple", 0);

    const demo::PizzaOrder got = protaco::ToObject<demo::PizzaOrder>(order.Serialize());
    std::string sauce;
    got.get_ext().GetString("secretSauce", &sauce);
    std::printf("PizzaOrder ext: secretSauce=%s, schema key in ext=%s\n",
        sauce.c_str(), order.get_mutable_ext().SetInt("orderId", 9) ? "accepted (bug)" : "rejected");
}

// 4. Errors: TryToObject never throws.
void ErrorHandling()
{
    const std::vector<uint8_t> bytes = demo::Heartbeat().Serialize();
    demo::TransferRequest wrong;
    protaco::Error error;
    if (!protaco::TryToObject(bytes.data(), bytes.size(), &wrong, &error))
        std::printf("decode failed: path='%s' what='%s'\n", error.path.c_str(), error.what.c_str());
}

// 5. Logs: ToString shows every field, ext entries under ext{...}.
void Logging()
{
    demo::TransferRequest request;
    request.set_tm("TM1");
    demo::MoveInfo move;
    move.set_from("SHELF1");
    move.set_to("TABLE3");
    request.get_mutable_moves().push_back(move);
    request.get_mutable_ext().SetInt("priority", 5);
    std::printf("%s\n", request.ToString().c_str());
    std::printf("%s\n", request.ToString(protaco::TextStyle::Pretty()).c_str());
}

}  // namespace

int main()
{
    SendAndReceive();
    FixedOnly();
    ExtraData();
    ErrorHandling();
    Logging();
    return 0;
}
