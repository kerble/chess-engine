import requests
import json
import chess
import subprocess
import time
API_TOKEN = "api key"  # Replace this with your actual token
headers = {
    "Authorization": f"Bearer {API_TOKEN}",
    "Content-Type": "application/json"
}

# Function to convert moves string to FEN
def moves_to_fen(moves: str, initial_fen="startpos") -> str:
    if initial_fen == "startpos":
        board = chess.Board()
    else:
        board = chess.Board(fen=initial_fen)

    move_list = moves.split()

    for move in move_list:
        try:
            board.push_uci(move)
        except ValueError:
            print(f"Invalid move: {move}")
            break

    return board.fen()

# Function to check for active games or listen for challenges
def listen_for_challenges_or_active_games():
    # First, check if the bot is already in an active game
    url_active_games = "https://lichess.org/api/account/playing"

    response = requests.get(url_active_games, headers=headers)
    
    if response.status_code == 200:
        active_games = response.json().get('nowPlaying', [])
        if active_games:
            game_id = active_games[0]['gameId']  # Get the first active game
            print(f"Bot is already in a game: {game_id}")
            return game_id
        else:
            print("No active games found.")
    else:
        print(f"Failed to check active games: {response.status_code} - {response.json()}")

    # If no active games, start listening for challenges
    url_challenges = "https://lichess.org/api/stream/event"

    response = requests.get(url_challenges, headers=headers, stream=True)

    if response.status_code == 200:
        print("Listening for incoming events...")
        # Stream the events
        for line in response.iter_lines():
            if line:
                event = json.loads(line.decode("utf-8"))
                print(f"Received event: {event}")

                # Check if the event is a challenge
                if event['type'] == 'challenge':
                    challenge_id = event['challenge']['id']
                    print(f"Accepting challenge: {challenge_id}")

                    # Accept the challenge
                    accept_url = f"https://lichess.org/api/challenge/{challenge_id}/accept"
                    accept_response = requests.post(accept_url, headers=headers)

                    if accept_response.status_code == 200:
                        print(f"Challenge {challenge_id} accepted successfully!")
                        return challenge_id
                    else:
                        print(f"Failed to accept challenge {challenge_id}: {accept_response.status_code} - {accept_response.json()}")
    else:
        print(f"Failed to listen for events: {response.status_code} - {response.json()}")

    return None


# Function to check for and accept incoming challenges
def listen_for_challenges():
    url = "https://lichess.org/api/stream/event"

    response = requests.get(url, headers=headers, stream=True)

    if response.status_code == 200:
        print("Listening for incoming events...")
        # Stream the events
        for line in response.iter_lines():
            if line:
                event = json.loads(line.decode("utf-8"))
                print(f"Received event: {event}")

                # Check if the event is a challenge
                if event['type'] == 'challenge':
                    challenge_id = event['challenge']['id']
                    print(f"Accepting challenge: {challenge_id}")

                    # Accept the challenge
                    accept_url = f"https://lichess.org/api/challenge/{challenge_id}/accept"
                    accept_response = requests.post(accept_url, headers=headers)

                    if accept_response.status_code == 200:
                        print(f"Challenge {challenge_id} accepted successfully!")
                        return challenge_id
                    else:
                        print(f"Failed to accept challenge {challenge_id}: {accept_response.status_code} - {accept_response.json()}")
    else:
        print(f"Failed to listen for events: {response.status_code} - {response.json()}")

    return None

# Main loop
def main():
    game_id = False
    while not game_id:
        game_id = listen_for_challenges_or_active_games()
        if game_id:
            print(f"Game ID of accepted challenge: {game_id}")
        
            url = f"https://lichess.org/api/bot/game/stream/{game_id}"

    # Variable to track which side Colbys_Bot is playing
    bot_side = None

    with requests.get(url, headers=headers, stream=True) as response:
        for line in response.iter_lines():
            if line:
                game_event = json.loads(line)

                if game_event['type'] == 'gameFull':
                    # Determine if Colbys_Bot is playing white or black
                    if game_event['white']['id'] == 'colbys_bot':
                        bot_side = 'white'
                    elif game_event['black']['id'] == 'colbys_bot':
                        bot_side = 'black'

                    initial_fen = game_event.get('initialFen', 'startpos')
                    moves = game_event['state']['moves']
                    fen = moves_to_fen(moves, initial_fen)

                    # If it's the bot's turn and there are no moves (first move of the game)
                    if moves == "" and bot_side == 'white':
                        run_process = subprocess.run(["./engine"] + fen.split(), capture_output=True, text=True)

                        if run_process.returncode == 0:
                            move = run_process.stdout.strip()

                            move_url = f"https://lichess.org/api/bot/game/{game_id}/move/{move}"

                            response = requests.post(move_url, headers=headers)

                            # if response.status_code == 200:
                            #     print("First move played successfully!")
                            # else:
                            #     print("First move failed:", response.json())

                elif game_event['type'] == 'gameState':
                    moves = game_event['moves']
                    fen = moves_to_fen(moves)

                    # Check whose turn it is (white plays on even number of moves, black on odd)
                    if (len(moves.split()) % 2 == 0 and bot_side == 'white') or (len(moves.split()) % 2 == 1 and bot_side == 'black'):
                        run_process = subprocess.run(["./engine"] + fen.split(), capture_output=True, text=True)

                        if run_process.returncode == 0:
                            move = run_process.stdout.strip()

                            move_url = f"https://lichess.org/api/bot/game/{game_id}/move/{move}"

                            response = requests.post(move_url, headers=headers)

if __name__ == "__main__":
    main()
