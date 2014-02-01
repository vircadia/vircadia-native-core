var timer = new Timer();
timer.interval = 1000;
timer.singleShot = true; // set this is you only want the timer to fire once
timer.timeout.connect(function() { print("TIMER FIRED!"); });
timer.start();
