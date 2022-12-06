import socket
import sys
from _thread import *
import random
import operator
import time

from player import Player

players = []
maxPlayer = 2
maxTime = None
numPlayers = 0
currentRound = 0

Operation = {
     "+": operator.add,
     "-": operator.sub,
     "*": operator.mul,
     "/": operator.floordiv,
     "%": operator.mod
}

def getOperation(i):
     switcher = ['+', '-', '*', '/', '%']
     return switcher[i]

def login(connection, ip, port):
     global maxPlayer
     global players
     global currentRound
     global maxTime

     connection.send(str.encode('Welcome to the Server'+"_"+str(maxPlayer) +"_"+str(maxTime)))
     isRegistered = False
     while isRegistered == False:
          reply = "something"

          message = connection.recv(1020)
          nickname = message.decode('utf-8')
          isRegistered = True

          nickname = nickname.rstrip(' \t\r\n\0')
          print("Length of nickname: {}".format(len(nickname)))

          if len(nickname) > 10:
               isRegistered = False
               reply = 'Nickname should be shorter than 10 characters'
          
          if isRegistered:
               for char in nickname:
                    if (char.isalnum() == False and char != '_'):
                         isRegistered = False
                         reply = 'Invalid character.'
                         break
                    
               # check duplicate 
          if isRegistered :
               if players != []:
                    for player in players:
                         if player.nickname == nickname:
                              isRegistered = False
                              reply = "Nickname existed"
                              break

          if isRegistered == True:
               print("Receive Player nickname: {}".format(nickname))
          
          if isRegistered == True:
               reply = 'ok_' + str(len(players))
               newPlayer = Player(connection, ip, port, nickname, color=len(players))
               players.append(newPlayer)

          connection.sendall(str.encode(reply))
          if isRegistered:
               currentRound += 1

def playNewRound(connection, player, a, b, operation, index):
     global currentRound
     global players
     
     player.timer = time.time()
     connection.sendall(str.encode(  str(a) + operation  + str(b)  )) 

     answer = connection.recv(1020).decode('utf-8') 
     answer = answer.rstrip(' \t\r\n\0')

     player.info()

     player.answer = int(answer)
     player.timer = time.time() - player.timer
     players[index] = player
     currentRound += 1

def init(connection, race):
     global currentRound
     connection.sendall(str.encode(str(race)))
     _ = connection.recv(1020)
     currentRound += 1

def updateGameStatus(connection, Message):
     global currentRound
     connection.sendall(str.encode(Message))
     _ = connection.recv(1020)
     currentRound += 1

def initServer(port = 1123, host = ''):
     global players
     global maxPlayer
     global maxTime
     global numPlayers
     global currentRound

     while True:
          isQuiz = False
          players = []
          maxPlayer = 2
          maxTime = None
          numPlayers = 0
          currentRound = 0

          while True:
               maxPlayer = int(input('Maximum numbers of players (2 to 10prs, input -1 to quit): '))
               if maxPlayer >= 2 and maxPlayer <=10:
                    break
               if maxPlayer == -1:
                    isQuiz = True 
                    break
          
          if isQuiz == True:
               print('Quiting game!. Good bye')
               break

          while True:
               maxTime = int(input('-> Input max time (10 to 15 seconds): '))
               if maxTime >= 10 and maxTime <=15:
                    break

          print("Maximum number of players: ", maxPlayer)
          print("Maximum time: ", maxTime)
          
          server = socket.socket()
          
          server.bind((host, port))
          print('Waiting for connection')
          server.listen(2)

          while True:
               client, address = server.accept()
               print('Connected to: ' + address[0] + ':' + str(address[1]))
               
               if numPlayers < maxPlayer: 
                    numPlayers += 1
                    print('Player Number: ' + str(numPlayers))
                    start_new_thread(login, (client, address[0], address[1]))
                    
               else:
                    client.sendall(str.encode("Full player"))
               if numPlayers == maxPlayer:
                    break
          
          print("Wait for all players", end='')
          while currentRound != maxPlayer:
               time.sleep(0.5)
               print('', end='')

          raceLength = input('Race length (4 to 25 rounds): ')
          while not (raceLength.isnumeric() and int(raceLength)>3 and int(raceLength)<26):
               raceLength = input('Race length (4 to 25 rounds): ')
          raceLength = int(raceLength)

          for index, player in enumerate(players):
               start_new_thread(init, (player.connection,raceLength))

          currentRound = 0
          while currentRound != maxPlayer:
               time.sleep(0.5)

          while True:
               isAlive = 0
               currentRound = 0

               a = random.randint(-10000,10000)
               operator = random.randrange(5)
               if operator == 3 or operator == 4:
                    b = random.randint(-10000, 10000) 
                    while b == 0:
                         b = random.randint(-10000, 10000) 
               else:
                    b = random.randint(-10,10)               
               operationCharacter = getOperation(operator)
               operationFunction = Operation[operationCharacter]

               result = operationFunction(a, b)
               print("Result: ",result)       
               
               for index, player in enumerate(players):
                    if player.isAlive:
                         isAlive += 1
                         start_new_thread(playNewRound, (player.connection, player, a, b, operationCharacter, index ))

               print("Wait for all players! ", end='')
               while currentRound != isAlive:
                    time.sleep(0.5)

               fastest_timer = float('inf')
               fastestPlayer = None

               winningGames = []
               isWinning = False
               fastestBonus = 0
               

               for index, player in enumerate(players):
                    if player.isAlive:
                         player.update(result, raceLength)
                    
                         if player.timer < fastest_timer and player.correct:
                              fastest_timer = player.timer
                              fastestPlayer = index

                         if player.correct == False:
                              fastestBonus += 1

               if fastestPlayer != None and fastestBonus > 0:
                    players[fastestPlayer].position += fastestBonus - 1

               message = []

               for player in players:
                    player.info()
                    if player.win:
                         winningGames.append(1)
                    else:
                         winningGames.append(0)

               for player in players: 
                    if player.win and sum(winningGames) == 1:
                         player.position = 100
                         isWinning = True
                    message.append(player.position)

               print(message)
               message = [str(s) for s in message]
               message = "_".join(message)

               for index, player in enumerate(players):
                    if player.isAlive:
                         start_new_thread(updateGameStatus, (player.connection, message) )
                         if player.position == -100:
                              player.isAlive = False

               currentRound = 0
               while currentRound != isAlive:
                    time.sleep(0.5)

               if isWinning:
                    print("We find winner")
                    break 
               if isAlive == 0:
                    print("All players lose")
                    break

          print("Game End - Next Race")
          port += 1
     
     if server != None:
          print("Closing connection")
          server.close()

if __name__ == '__main__':
     initServer()