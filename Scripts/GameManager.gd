extends Node

# Global Game Manager
# Handles core game state, session management, and global inputs.

var game_state: String = "INITIALIZING"
var player_count: int = 0

func _ready():
	print("D&D Game Engine Initialized")
	game_state = "MAIN_MENU"
