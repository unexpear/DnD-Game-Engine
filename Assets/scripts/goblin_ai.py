import sys, json

name = "unknown"

for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    try:
        event = json.loads(line)
    except json.JSONDecodeError:
        continue

    evt_type = event.get("type", "")

    if evt_type == "init":
        name = event.get("name", "goblin")
        response = {"type": "log", "message": f"{name} the goblin stirs..."}
        sys.stdout.write(json.dumps(response) + "\n")
        sys.stdout.flush()

    elif evt_type == "damage":
        amt = event.get("amount", 0)
        hp = event.get("hp", 0)
        if hp <= 0:
            response = {"type": "log", "message": f"{name} screeches in pain and falls!"}
        elif hp < 5:
            response = {"type": "log", "message": f"{name} snarls, backing away!"}
        else:
            response = {"type": "log", "message": f"{name} grunts and presses the attack!"}
        sys.stdout.write(json.dumps(response) + "\n")
        sys.stdout.flush()

    elif evt_type == "death":
        response = {"type": "log", "message": f"{name} lets out a final gurgle and goes still."}
        sys.stdout.write(json.dumps(response) + "\n")
        sys.stdout.flush()

    elif evt_type == "update":
        phase = event.get("phase", "")
        if phase == "combat_start":
            response = {"type": "log", "message": f"{name} snickers, drawing a dagger."}
        elif phase == "combat_end":
            response = {"type": "log", "message": f""}
        else:
            response = {"type": "log", "message": f"{name} watches you carefully."}
        sys.stdout.write(json.dumps(response) + "\n")
        sys.stdout.flush()
