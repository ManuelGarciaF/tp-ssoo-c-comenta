#!/usr/bin/env bash

tmux kill-session -t tp-ssoo 2> /dev/null

tmux new-session -d -s tp-ssoo

# Compilar todo
tmux rename-window "Memoria"
tmux send-keys -t tp-ssoo:1 "cd memoria && make && make memcheck" C-m

tmux new-window -t tp-ssoo:2 -n "CPU"
tmux send-keys -t tp-ssoo:2 "cd cpu && make" C-m

tmux new-window -t tp-ssoo:3 -n "Kernel"
tmux send-keys -t tp-ssoo:3 "cd kernel && make" C-m

tmux new-window -t tp-ssoo:4 -n "IO"
tmux send-keys -t tp-ssoo:4 "cd entradasalida && make" C-m

# Esperar que memoria inicie
sleep 1
tmux send-keys -t tp-ssoo:2 "make memcheck" C-m # Correr cpu

# Esperar que CPU inicie
sleep 1
# Correr kernel
if [[ -z $1 ]]; then
    tmux send-keys -t tp-ssoo:3 "make memcheck" C-m
else
    tmux send-keys -t tp-ssoo:3 "../$1 | make memcheck" C-m
fi

# Esperar que kernel inicie
sleep 1.5
# Correr IO
tmux send-keys -t tp-ssoo:4 "valgrind --leak-check=full ./bin/entradasalida ESPERA ESPERA.config" C-m

tmux select-window -t tp-ssoo:3
tmux attach-session -t tp-ssoo
