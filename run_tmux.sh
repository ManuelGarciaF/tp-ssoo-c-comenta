#!/usr/bin/env bash

tmux kill-session -t tp-ssoo 2> /dev/null

tmux new-session -d -s tp-ssoo

if [[ $1 == "release" ]]; then
    echo "Compilando en modo release"
    export MAKE="make release --always-make"
else
    echo "Compilando en modo debug"
    export MAKE="make --always-make"
fi

# Compilar todo
tmux new-window -t tp-ssoo:1 -k -n "Memoria"
tmux send-keys -t tp-ssoo:1 "cd memoria && $MAKE && make memcheck" C-m

tmux new-window -t tp-ssoo:2 -n "CPU"
tmux send-keys -t tp-ssoo:2 "cd cpu && $MAKE" C-m

tmux new-window -t tp-ssoo:3 -n "Kernel"
tmux send-keys -t tp-ssoo:3 "cd kernel && $MAKE" C-m

tmux new-window -t tp-ssoo:4 -n "IO"
tmux send-keys -t tp-ssoo:4 "cd entradasalida && $MAKE" C-m

# Esperar que memoria inicie
sleep 1.5
tmux send-keys -t tp-ssoo:2 "make memcheck" C-m # Correr cpu

# Esperar que CPU inicie
sleep 1.5
tmux send-keys -t tp-ssoo:3 "make memcheck" C-m # Correr kernel

# Escribir el inicio del comando
tmux send-keys -t tp-ssoo:4 "valgrind --leak-check=full ./bin/entradasalida "

tmux select-window -t tp-ssoo:3
tmux attach-session -t tp-ssoo
