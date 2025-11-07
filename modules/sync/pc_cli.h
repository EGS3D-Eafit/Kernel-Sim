#pragma once
#include <string>
#include <optional>

namespace pc_cli
{

    // Initialize or reset the global buffer capacity (default 5).
    void init(size_t capacity = 5);

    // Produce one value (if not provided, auto-incrementing counter is used). Returns a message.
    std::string produce(std::optional<int> value = std::nullopt);

    // Consume one value if available (non-blocking). Returns a message.
    std::string consume();

    // Return a one-line status string of the buffer.
    std::string stat();

}
