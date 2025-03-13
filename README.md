# Projet Entrepot

A project aiming to answer the problem of how to automatically serve clients commands in a warehouse.

## Different systems

To fulfill this goal, the whole project is composed of several actors/systems. 
- The `inventaire` supposed to manage the stock of the warehouse and allow clients to pass commands.
- The robots, the code is aimed to control Makeblock Ultimate robots. They are composed of a Raspberry pi 3 and an Arduino Mega.
- The `ordinateur central` creates the different waypoints inside the warehouse, manages the ressources and controls the different robots.

A SysML explanation of the system is available in this [repository](https://github.com/Zakurama/projet_entrepot_sysml)

## Connexions between systems

To launch the project, the `ordinateur central` must be launched first, as it is a TCP server for both the robots and the `inventaire`.

`Inventaire` must then be launched to connect to `ordinateur central` and accept clients connexions.
In order for a client to do a request, a minimum of 1 robot should be connected.
The `inventaire` also allow workers to manage the warehouse stocks.

## Dependencies

### Tests

The test framework for this project is [CUnit](https://cunit.sourceforge.net/), to install it do the following

```shell
sudo apt install libcunit1 libcunit1-dev  # Debian/Ubuntu
sudo dnf install cunit-devel              # Fedora
sudo pacman -S cunit                      # Arch Linux
```

### Robots

For the robot code, to use the gyroscope, it is needed to download the [Makeblock librairy](https://github.com/Makeblock-official/Makeblock-Libraries) and install it in Arduino.
