#/usr/bin/env bash

#sudo fuser --kill 8002/tcp
#sudo fuser --kill 8006/tcp
#sudo fuser --kill 8007/tcp

# Check if the session already exists
if tmux has-session -t tp-ssoo 2>/dev/null; then
    # If the session exists, kill it
    tmux kill-session -t tp-ssoo
fi

tmux new-session -d -s tp-ssoo

tmux rename-window "Memoria"
tmux send-keys -t tp-ssoo:1 "cd memoria && make && make memcheck" C-m

# Esperar que memoria inicie
sleep 2

tmux new-window -t tp-ssoo:2 -n "CPU"
tmux send-keys -t tp-ssoo:2 "cd cpu && make && make memcheck" C-m

# Esperar que cpu inicie
sleep 2

tmux new-window -t tp-ssoo:3 -n "Kernel"
tmux send-keys -t tp-ssoo:3 "cd kernel && make && make memcheck" C-m
sleep 2
# tmux send-keys -t tp-ssoo:3 "INICIAR_PROCESO test.txt" C-m

# Attach to the session
tmux attach-session -t tp-ssoo
