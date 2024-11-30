#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>

// Functions for engine logic
std::string get_best_move(const std::string& fen);
void apply_move(const std::string& move);

// Initialization
void initialize_engine();

#endif // ENGINE_HPP
