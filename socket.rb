require 'socket'
BUFFER_SLEEP_DURATION_MSECS = 20
CLIENT_LIST_MEMORY_DURATION_SECS = 1

begin 
  # create the UDPSocket object
  socket = UDPSocket.new

  $dataBuffer = nil
  $receivers = Hash.new

  listenThread = Thread.new(socket) {
    # bind it to 0.0.0.0 at the given port
    port = 55443
    socket.bind "0.0.0.0", port
    puts "UDP Socket bound to port #{port} and listening."

    # while true loop to keep listening for new packets
    while true do
      data, sender = socket.recvfrom 1024
      puts "Recieved #{data.size} bytes from #{sender[3]} on #{sender[1]}"

      # there are 2 bytes per sample
      newSamples = data.unpack("s<*")
      
      if $dataBuffer.nil?
        $dataBuffer = newSamples
      else
        $dataBuffer.each_with_index do |sample, index|
          sample = (sample + newSamples[index]) / 2
        end
      end

      $receivers["#{sender[3]}:#{sender[1]}"] = Time.now.to_f
    end  
  }

  sendThread = Thread.new(socket) {
    while true do
      sleep(BUFFER_SLEEP_DURATION_MSECS/1000)
      
      unless $dataBuffer.nil?
        $receivers.each do |ip_port, time|
          if (time > Time.now.to_f - CLIENT_LIST_MEMORY_DURATION_SECS)
            ip, port = ip_port.split(':')
            socket.send $dataBuffer.pack("s<*"), 0, ip, port
            puts "Sent mixed frame to #{ip} on #{port}"
          else
            puts "Nobody to send mixed frame to"
            $receivers.delete(ip_port)
          end
        end

        $dataBuffer = nil
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
