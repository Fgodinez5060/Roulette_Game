# DE-10 Roulette Game - Final Release

**Author:** Fernando Godinez  
**Course:** SWE-450 Embedded Systems  
**Date:** April 2026

## Project Description
This project implements a roulette game on the DE-10 Standard board using the ARM HPS processor and FPGA fabric. Players use physical switches to select bet types and amounts, LEDs visualize the spinning roulette wheel, HEX displays show the current balance in real-time, the built-in 128x64 LCD displays game instructions and results, and push buttons control the game flow.

## Features
- Multiple bet types: Number (10x payout), Odd/Even (2x), High/Low (2x)
- 10 LED roulette wheel with spin animation
- 128x64 LCD display showing bet instructions, live switch feedback, and results
- Real-time balance on 7-segment HEX displays
- Switch-based input for bet type, amount, and position selection
- Button controls for spin, reset, check balance, and quit
- FPGA LFSR hardware random number generator

## FPGA Circuits
1. BCD to 7-Segment Decoder — drives HEX0-HEX5 from 24-bit BCD register
2. LFSR Random Number Generator — 32-bit hardware pseudo-random generator at address 0x6000

## Controls
- KEY0: Play / Confirm
- KEY1: Reset game
- KEY2: Check balance
- KEY3: Quit
- SW0-SW3: Bet amount ($10/$20/$50/$100)
- SW8-SW9: Bet type (both off = Number, SW8 = Odd/Even, SW9 = High/Low)
- SW0-SW9: Number selection (for Number bets)

## Build Instructions
```
make clean
make
sudo ./roulette_game
```

```
sudo ./roulette_game
```
