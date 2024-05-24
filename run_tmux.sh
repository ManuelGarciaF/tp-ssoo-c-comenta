#!/usr/bin/env bash

tmux kill-session -t tp-ssoo 2> /dev/null

tmux new-session -d -s tp-ssoo

tmux rename-window "Memoria"
tmux send-keys -t tp-ssoo:1 "cd memoria && make && make memcheck" C-m

# Esperar que memoria inicie
sleep 1

tmux new-window -t tp-ssoo:2 -n "CPU"
tmux send-keys -t tp-ssoo:2 "cd cpu && make && make memcheck" C-m

# Esperar que cpu inicie
sleep 1

tmux new-window -t tp-ssoo:3 -n "Kernel"
# tmux send-keys -t tp-ssoo:3 "cd kernel && make && ./bin/kernel" C-m
# tmux send-keys -t tp-ssoo:3 "cd kernel && make && make memcheck" C-m
tmux send-keys -t tp-ssoo:3 "cd kernel && make && ../tests/finalizar_proceso_io.py | make memcheck" C-m
sleep 1

tmux new-window -t tp-ssoo:4 -n "IO"
tmux send-keys -t tp-ssoo:4 "cd entradasalida && make" C-m
tmux send-keys -t tp-ssoo:4 "valgrind --leak-check=full ./bin/entradasalida Interfaz1 generica_ejemplo.config" C-m

# Select the kernel window
tmux select-window -t tp-ssoo:3

# Attach to the session
tmux attach-session -t tp-ssoo
