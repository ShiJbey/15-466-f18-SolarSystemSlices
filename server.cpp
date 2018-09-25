#include "Connection.hpp"
#include "SolarSystemSlicesGame.hpp"

#include <iostream>
#include <set>
#include <chrono>
#include <map>
#include <vector>

// Divide up connections to the sever into parties of two
struct Lobby {
	SolarSystemSlices::SolarSystemSlicesGame state;
	Connection* player_1_connection = nullptr;
	Connection* player_2_connection = nullptr;
	bool player_1_ready;
	bool player_2_ready;
};

// Deletes a specified lobby from the vector of lobbies
void remove_lobby(std::vector< Lobby *> &lobbies, Lobby* lobby_to_delete) {
	for (uint32_t i = 0; i < lobbies.size(); i++) {
		if (lobbies[i] == lobby_to_delete) {
			delete lobbies[i];
			lobbies.erase(lobbies.begin() + i);
			return;
		}
	}
}

// Returns a pointer to a lobby with the given conncetion
Lobby* get_lobby(std::vector< Lobby* > &lobbies, Connection *c) {
	for (uint32_t i = 0; i < lobbies.size(); i++) {
		if (c == lobbies[i]->player_1_connection || 
			c == lobbies[i]->player_2_connection) {
			
			return lobbies[i];
		}
	}
	return nullptr;
}

// Finds the next open lobby for a new connection
Lobby* get_open_lobby(std::vector< Lobby* > &lobbies, Connection *c) {
	for (uint32_t i = 0; i < lobbies.size(); i++) {
		if (lobbies[i]->player_1_connection == nullptr || 
			lobbies[i]->player_2_connection == nullptr) {
			
			return lobbies[i];
		}
	}
	return nullptr;
}

// Add the connection to the first open lobby or creates a new
// lobby and adds the connection
Lobby* add_to_lobby(std::vector< Lobby* > &lobbies, Connection *c) {
	Lobby* open_lobby = get_open_lobby(lobbies, c);
	if (open_lobby == nullptr) {
		// Create a new lobby and add this person
		Lobby* fresh_lobby = new Lobby();
		fresh_lobby->player_1_connection = c;
		lobbies.push_back(fresh_lobby);
		return fresh_lobby;
	} else {
		// Check which connection is open and add it
		if (open_lobby->player_1_connection == nullptr) {
			open_lobby->player_1_connection = c;
		}
		else {
			open_lobby->player_2_connection = c;
		}
		return open_lobby;
	}
}


int main(int argc, char **argv) {

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}
	
	// Server manages connections with hosts and doesn't care about
	// the state or the information being passed
	Server server(argv[1]);

	// Maintains each one of the two player games 
	std::vector< Lobby* > lobbies;


    std::cout << "Server Running" << std::endl;
	

	SolarSystemSlices::SolarSystemSlicesGame state;

	while (1) {

		server.poll([&](Connection *c, Connection::Event evt){
			if (evt == Connection::OnOpen) {
			} else if (evt == Connection::OnClose) {
				// Remove this connection from its lobby
				Lobby* lobby = get_lobby(lobbies, c);
				if (c == lobby->player_1_connection) {
					lobby->player_1_connection = nullptr;
				} else {
					lobby->player_2_connection = nullptr;
				}

				// Close the lobby when both players leave
				if (lobby->player_1_connection == nullptr && lobby->player_2_connection == nullptr) {
					remove_lobby(lobbies, lobby);
					std::cout << "Closing lobby..." << std::endl; 
				}

			} else { assert(evt == Connection::OnRecv);

				
				if (c->recv_buffer[0] == 'D') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);

					Lobby* lobby = add_to_lobby(lobbies, c);

					if (c == lobby->player_1_connection) {
						lobby->state.player_1_finished = true;
					} else {
						lobby->state.player_2_finished = true;
					}

					if (lobby->state.player_1_finished && lobby->state.player_2_finished) {
						uint32_t winner = lobby->state.get_winner();
						//std::cout << "Winner is: " << winner << std::endl;
						lobby->player_1_connection->send_raw("R", 1);
						lobby->player_2_connection->send_raw("R", 1);
						lobby->player_1_connection->send_raw(&winner, sizeof(uint32_t));
						lobby->player_2_connection->send_raw(&winner, sizeof(uint32_t));
					}
				}

				if (c->recv_buffer[0] == 'H') {
					if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
						return;
                	} else {
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
						uint32_t ingredient = -1U;
                    	memcpy(&ingredient, c->recv_buffer.data(), sizeof(uint32_t));
                    	c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

						Lobby* lobby = add_to_lobby(lobbies, c);

						// Find it
						for (uint32_t i = 0; i < lobby->state.floating_ingredients.size(); i++) {
							if (ingredient == lobby->state.floating_ingredients[i].id) {
								// Add it to the players map of collected ingredients
								if (c == lobby->player_1_connection) {
									if (lobby->state.player_1.collected_ingredients.find(lobby->state.floating_ingredients[i].name) == lobby->state.player_1.collected_ingredients.end()) {
										lobby->state.player_1.collected_ingredients.insert({lobby->state.floating_ingredients[i].name, 1});
									} else {
										lobby->state.player_1.collected_ingredients[lobby->state.floating_ingredients[i].name]++;
									}
								} else {
									if (lobby->state.player_2.collected_ingredients.find(lobby->state.floating_ingredients[i].name) == lobby->state.player_2.collected_ingredients.end()) {
										lobby->state.player_2.collected_ingredients.insert({lobby->state.floating_ingredients[i].name, 1});
									} else {
										lobby->state.player_2.collected_ingredients[lobby->state.floating_ingredients[i].name]++;
									}
								}
								// Remove it from the floating ingredients
								lobby->state.floating_ingredients.erase(lobby->state.floating_ingredients.begin() + i);
								lobby->player_1_connection->send_raw("H", 1);
								lobby->player_2_connection->send_raw("H", 1);
								lobby->player_1_connection->send_raw(&ingredient, sizeof(uint32_t));
								lobby->player_2_connection->send_raw(&ingredient, sizeof(uint32_t));
							}
						}

					}
				}
				

				if (c->recv_buffer[0] == 'h') {
					// Keep 'h' as the general "hello" code for a new connection
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
					
					std::cout << c << ": Got hello." << std::endl;
					std::cout << c << ": Looking for Lobby..." << std::endl;

					Lobby* lobby = add_to_lobby(lobbies, c);

					int player_number = -1;
					if (c == lobby->player_1_connection) {
						player_number = 1;
					} else {
						player_number = 2;
					}

					std::cout << c << ": Connected to a lobby as player: " << player_number << "..." << std::endl;
					c->send_raw("N", 1);
					c->send_raw(&player_number, sizeof(player_number));

					// Initiate the game when the lobby is full
					if (lobby->player_1_connection != nullptr && lobby->player_2_connection != nullptr) {

						{ // Send signal to the players that both players are connected
							std::cout << "Both Players Connected" << std::endl;
							lobby->player_1_connection->send_raw("C", 1);
							lobby->player_2_connection->send_raw("C", 1);
						}
						
						{ // Create a new pizza order and send it to the clients
							lobby->state.generate_order();
							lobby->state.print_order();

							// Send both players the "order" 'O' signal;
							lobby->player_1_connection->send_raw("O", 1);
							lobby->player_2_connection->send_raw("O", 1);

							// Send players the number of items in the order
							uint32_t num_ingredients = (uint32_t)lobby->state.current_order.size();
							lobby->player_1_connection->send_raw(&num_ingredients, sizeof(num_ingredients));
							lobby->player_2_connection->send_raw(&num_ingredients, sizeof(num_ingredients));

							// Send the first character of each of the ingredients
							for (uint32_t i = 0; i < lobby->state.current_order.size(); i++) {
								lobby->player_1_connection->send_raw(&lobby->state.current_order[i][0], 1);
								lobby->player_2_connection->send_raw(&lobby->state.current_order[i][0], 1);
							}
							std::cout << std::endl;
						}	

						{ // Spawn all of the asteroids in the game and send them to clients
							uint32_t num_asteroids = 50;
							lobby->state.populate_asteroids(num_asteroids);

							// Send the signal for asteroid data
							lobby->player_1_connection->send_raw("A", 1);
							lobby->player_2_connection->send_raw("A", 1);

							// Send number of asteroids
							lobby->player_1_connection->send_raw(&num_asteroids, sizeof(uint32_t));
							lobby->player_2_connection->send_raw(&num_asteroids, sizeof(uint32_t));

							// Send Each asteroids x,y
							for (uint32_t i = 0; i < num_asteroids; i++) {
								lobby->player_1_connection->send_raw(&lobby->state.asteroids[i].position.x, sizeof(float));
								lobby->player_2_connection->send_raw(&lobby->state.asteroids[i].position.x, sizeof(float));
								
								lobby->player_1_connection->send_raw(&lobby->state.asteroids[i].position.y, sizeof(float));
								lobby->player_2_connection->send_raw(&lobby->state.asteroids[i].position.y, sizeof(float));
							}
						}

						{ // Spawn all of the ingredients and send them to clients
							uint32_t num_of_each_ingredient = 10;
							uint32_t total_ingredients = (uint32_t)lobby->state.current_order.size() * num_of_each_ingredient;
							lobby->state.populate_ingredients(num_of_each_ingredient);

							assert(total_ingredients == lobby->state.floating_ingredients.size());

							// Send the signal for ingedient data
							lobby->player_1_connection->send_raw("I", 1);
							lobby->player_2_connection->send_raw("I", 1);

							// Send total number of ingredients
							lobby->player_1_connection->send_raw(&total_ingredients, sizeof(uint32_t));
							lobby->player_2_connection->send_raw(&total_ingredients, sizeof(uint32_t));

							// Send the Data for each ingredient
							for (uint32_t i = 0; i < lobby->state.floating_ingredients.size(); i++) {
								lobby->player_1_connection->send_raw(&lobby->state.floating_ingredients[i].name[0], sizeof(char));
								lobby->player_2_connection->send_raw(&lobby->state.floating_ingredients[i].name[0], sizeof(char));

								lobby->player_1_connection->send_raw(&lobby->state.floating_ingredients[i].id, sizeof(uint32_t));
								lobby->player_2_connection->send_raw(&lobby->state.floating_ingredients[i].id, sizeof(uint32_t));

								lobby->player_1_connection->send_raw(&lobby->state.floating_ingredients[i].position.x, sizeof(float));
								lobby->player_2_connection->send_raw(&lobby->state.floating_ingredients[i].position.x, sizeof(float));
								
								lobby->player_1_connection->send_raw(&lobby->state.floating_ingredients[i].position.y, sizeof(float));
								lobby->player_2_connection->send_raw(&lobby->state.floating_ingredients[i].position.y, sizeof(float));
							}
						}					
						
						lobby->state.game_started = true;
					}
				}
				
				if (c->recv_buffer[0] == 's') {
					// Keep 's' as the state update code
					if (c->recv_buffer.size() < 1 + sizeof(float) * 2) {
						return; //wait for more data
					} else {
						Lobby *lobby = get_lobby(lobbies, c);
						if (lobby != nullptr) {
							if (c == lobby->player_1_connection) {
								// Update the internal state
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
								memcpy(&lobby->state.player_1.position.x, c->recv_buffer.data(), sizeof(float));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);
								memcpy(&lobby->state.player_1.position.y, c->recv_buffer.data(), sizeof(float));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);

								if (lobby->player_2_connection != nullptr) {
									// Send update information to the other player
									lobby->player_2_connection->send_raw("s", 1);
									lobby->player_2_connection->send_raw(&lobby->state.player_1.position.x, sizeof(float));
									lobby->player_2_connection->send_raw(&lobby->state.player_1.position.y, sizeof(float));
								}
							}
							else if(c == lobby->player_2_connection) {
								// Update the internal state
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
								memcpy(&lobby->state.player_2.position.x, c->recv_buffer.data(), sizeof(float));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);
								memcpy(&lobby->state.player_2.position.y, c->recv_buffer.data(), sizeof(float));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);

								if (lobby->player_1_connection != nullptr) {
									// Send update information to the other player
								lobby->player_1_connection->send_raw("s", 1);
								lobby->player_1_connection->send_raw(&lobby->state.player_2.position.x, sizeof(float));
								lobby->player_1_connection->send_raw(&lobby->state.player_2.position.y, sizeof(float));
								}
							}
						}
						else {
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float) + sizeof(float));
						}
					}
				}

				if(c->recv_buffer[0] == 'P') {
					if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
						return;
					} else {
						uint32_t player_planet_name_length;
						memcpy(&player_planet_name_length, c->recv_buffer.data() + 1, sizeof(uint32_t));

						if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + (1 * player_planet_name_length)) {
							return;
						} else {
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));
							
							char *planet_name = new char[player_planet_name_length + 1];
							planet_name[player_planet_name_length] = '\0';
							memcpy(planet_name, c->recv_buffer.data(), player_planet_name_length);

							// Get the lobby for this connection
							Lobby* lobby = get_lobby(lobbies, c);
							
							if (lobby != nullptr) {
								if (c == lobby->player_1_connection) {
									lobby->player_1_ready = true;
									lobby->state.player_1.planet = std::string(planet_name);
									std::cout << "Player 1 Representing: " << lobby->state.player_1.planet << std::endl;
								} else {
									lobby->player_2_ready = true;
									lobby->state.player_2.planet = std::string(planet_name);
									std::cout << "Player 2 Representing: " << lobby->state.player_2.planet << std::endl;
								}
							}

							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + player_planet_name_length);

							// When the last player is ready, send ready signals to both players
							if (lobby->player_1_ready && lobby->player_2_ready) {
								uint32_t planet_name_length;

								// Broadcast to player 1
								lobby->player_1_connection->send_raw("P", 1);
								planet_name_length = (uint32_t)lobby->state.player_2.planet.size();
								lobby->player_1_connection->send_raw(&planet_name_length, sizeof(uint32_t));
								lobby->player_1_connection->send_raw(&lobby->state.player_2.planet, planet_name_length);

								// Broadcast to player 2
								lobby->player_2_connection->send_raw("P", 1);
								planet_name_length = (uint32_t)lobby->state.player_1.planet.size();
								lobby->player_2_connection->send_raw(&planet_name_length, sizeof(uint32_t));
								lobby->player_2_connection->send_raw(&lobby->state.player_1.planet, planet_name_length);
							}
							
							
						}
					}
				}

			}
		}, 0.01);

	}
}
