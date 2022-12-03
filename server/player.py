class Player:
     def __init__(self, connection, ip, port, nickname, color):
          self.connection = connection
          self.ip = ip
          self.port = port
          self.position = 1
          self.nickname = nickname
          self.isAlive = True
          self.isFailedThreeTimes = 0
          self.answer = None
          self.win = False
          self.timer = 0
          self.color = color
          self.correct = False

     def info(self):
          print("*****************************")
          print("Position: ",self.position)
          print("Nickname: ",self.nickname)
          print("Wrong 3 times: ",self.isFailedThreeTimes)
          print("Winner: ",self.win)
          print("Correct: ",self.correct)
          print("*****************************")

     def update(self, result, race):
          if self.answer == result:
               self.position += 1
               self.isFailedThreeTimes = 0
               if self.checkIsWinning(race):
                    self.win = True
               self.correct = True
          else:
               if self.position > 1:
                    self.position -= 1
               self.isFailedThreeTimes += 1
               if self.checkIsFailedThreeTimes():
                    self.position = -100
               self.correct = False

     def checkIsFailedThreeTimes(self):
          if self.isFailedThreeTimes == 3:
               return True
          else:
               return False

     def checkIsWinning(self, race):
          if self.position > race:
               return True
          else:
               return False