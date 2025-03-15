#!/bin/bash

# Create directories if they don't exist
mkdir -p resources/icons resources/logos

# Download app icon
curl -L "https://cdn-icons-png.flaticon.com/512/3097/3097180.png" -o resources/icons/motorsportrss.png

# Download motorsport logos
# Formula 1
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/3/33/F1.svg/240px-F1.svg.png" -o resources/logos/f1.png

# MotoGP
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/e/ea/MotoGP_Logo.svg/240px-MotoGP_Logo.svg.png" -o resources/logos/motogp.png

# WRC
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/3/3a/WRC_Logo.svg/240px-WRC_Logo.svg.png" -o resources/logos/wrc.png

# NASCAR
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/5/5c/NASCAR_logo.svg/240px-NASCAR_logo.svg.png" -o resources/logos/nascar.png

# IndyCar
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/4/40/IndyCar_Series_logo_%282018%29.svg/240px-IndyCar_Series_logo_%282018%29.svg.png" -o resources/logos/indycar.png

# WEC
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/0/0f/FIA_World_Endurance_Championship_logo.svg/240px-FIA_World_Endurance_Championship_logo.svg.png" -o resources/logos/wec.png

# Formula E
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/2/22/FIA_Formula_E_logo.svg/240px-FIA_Formula_E_logo.svg.png" -o resources/logos/formula-e.png

# DTM
curl -L "https://upload.wikimedia.org/wikipedia/commons/thumb/3/32/DTM_logo.svg/240px-DTM_logo.svg.png" -o resources/logos/dtm.png

echo "Logos downloaded successfully!" 