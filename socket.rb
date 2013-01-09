require 'socket'
begin 
  # create the UDPSocket object
  sock = UDPSocket.new
  
  # bind it to 0.0.0.0 at the given port
  port = 55443
  sock.bind "0.0.0.0", port
  puts "UDP Socket bound to port #{port} and listening."

  # while true loop to keep listening for new packets
  while true do
    data, sender = sock.recvfrom 1024
    # puts "#{sender[3]} sent #{data} on port #{sender[1]}"
    puts "Recieved #{data.size} from #{sender[3]}"
    rand_photo_url = "http://d4gpsb1dbo4rf.cloudfront.net/photo#{rand(11)}.jpg"
    puts "Sending #{rand_photo_url.size} back"
    sock.send rand_photo_url, 0, sender[3], sender[1]
  end  

  rescue SystemExit, Interrupt => e
    puts
    puts "Okay, fine. I'll stop listening."
    sock.close
end  
