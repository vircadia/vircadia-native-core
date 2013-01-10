require 'socket'
BUFFER_SLEEP_DURATION_MSECS = 20

begin 
  # create the UDPSocket object
  socket = UDPSocket.new

  $dataBuffer = Array.new
  $receivers = Hash.new

  listenThread = Thread.new(socket) {
    # bind it to 0.0.0.0 at the given port
    port = 55443
    socket.bind "0.0.0.0", port
    puts "UDP Socket bound to port #{port} and listening."

    # while true loop to keep listening for new packets
    while true do
      data, sender = socket.recvfrom 1024
      puts "Recieved #{data.size} bytes from #{sender[3]}"
      $dataBuffer << data
      $receivers[sender[3]] = sender[1]
    end  
  }

  sendThread = Thread.new(socket) {
    while true do
      sleep(BUFFER_SLEEP_DURATION_MSECS/1000)
      
      if $dataBuffer.size > 0 
        $receivers.each do |ip, port|
          puts $dataBuffer[0]
          puts ip
          puts port
          socket.send $dataBuffer[0], 0, ip, port
        end

        $dataBuffer.clear
      end      
    end
  }

  listenThread.join
  sendThread.join

  rescue SystemExit, Interrupt => e
    puts
    puts "Okay, fine. I'll stop listening."
    socket.close
end  
