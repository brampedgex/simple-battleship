cmake_minimum_required(VERSION 3.11)
project(battleship)

add_executable(battleship main.c board.c player.c network.c util.c)
