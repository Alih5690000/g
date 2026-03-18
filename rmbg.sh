#!/bin/bash
convert "$1"/*.png -transparent "#FFFFFF" "$2"/frame%d.png