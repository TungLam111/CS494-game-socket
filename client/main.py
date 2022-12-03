import socket, time, sys
import time

answer = None

def client(port = 1123, host = '127.0.0.1'):
     client = socket.socket()     

     print('Waiting for connection')
     client.connect((host, port))

     message = client.recv(1020).decode('utf-8')

     color = None
     raceLength = None
     lastStatus = 1
     isPlaying = True
     port += 1

     while True:

          if isPlaying == False:
               break

          isLoggedInSuccessfully = False
          while not isLoggedInSuccessfully:
               nickname = input('Your nickname: ')
               client.send(str.encode(nickname))

               message = client.recv(1020)
               
               if message.decode('utf-8')[:2] == "ok":
                    color = int(message.decode('utf-8').split("_")[1])
                    print("Registration completed successfully with order: ", color)
                    isLoggedInSuccessfully = True
               else:
                    print(message.decode('utf-8'))

          print("Wait for another player")
          message = client.recv(1020)
          raceLength = int(message)

          time.sleep(2)

          print("Race length: " + str(raceLength))
          client.send(str.encode("Ready"))

          while True:
               message = client.recv(1020)
               print(message.decode('utf-8'))

               answer = input('Your answer: ')
               client.send(str.encode(answer))
               answer = None

               print("Wait for another racer")
               message = client.recv(1020).decode('utf-8')

               print(message)
               gameStatus = message.split("_")
               gameStatus = [int(s) for s in gameStatus]

               playerStatus = gameStatus[color]

               if playerStatus == -100:
                    print("You lose")
               elif playerStatus == 100:
                    print("You win")
               else:
                    if playerStatus == lastStatus + 1:
                         print()
                    elif playerStatus == lastStatus - 1:
                         print()
                    elif playerStatus > lastStatus:
                         print("Bonus for Fastest Racer in this round")

               lastStatus = playerStatus

               
               if playerStatus == 100:
                    isPlaying = False
                    client.send(str.encode("Win game"))
                    break
               elif playerStatus == -100:
                    isPlaying = False
                    client.send(str.encode("Out game"))
                    break
               else:
                    client.send(str.encode("Next question"))
                    print("Next round")
     client.close()

if __name__ == '__main__':
     client()