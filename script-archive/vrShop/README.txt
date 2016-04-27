Instructions for experiencing vrShop
Alessandro Signa and Edgar Pironti, 13 Jan 2016

To avoid weird issues we suggest to run this build https://github.com/highfidelity/hifi/pull/6786
To test the experience in the current release of interface that PR needs to be merged 
and the scripts need to be updated to the new collisionMask system.

Go to the vrShop domain.

When you get closer to the shop (entering the Zone - GrabSwapper) you should notice that 
shopItemGrab is in your running scripts. At this point the handControllerGrab should be disabled
(it will be re-enabled automatically when leaving the zone), but if you still see rays 
coming out from your hands when you squeeze the triggers something went wrong so you probably 
want to remove manually handControllerGrab

Once you're here you can do two different experiences: buyer and reviewr. 
Both of them are designed to be done entirely using HMD and hand controllers, 
so please put your Oculus on and equip your hydra.

BUYER
First of all you have to pass through the pick cart line, the Zone - CartSpawner is there. 
Now you see a cart following you (look at your right).
Reach the shelf and grab an item (you can only near grab here).
An inspection area should now stay in front of you, drop the item inside it to inspect. Be sure
to see the overlay turning green before releasing the item.

In inspect mode you can't move or rotate your avatar but you can scale and rotate the item.
Interact with the UI buttons using your bumpers: you can change the color of the item,
see the reviews (looking at the review cabin) and you can also try some of the items on (but 
this feature at the moment works well only with the Will avatar increasing its default size once).

If you are good with your item you can put it in the cart, otherwise just drop it around.
You can also pick again an item from your cart to inspect it or to discard it.
To empty the cart you can push the right primary button.

Then you can go to the cashier for the "payment": once you enter the Zone - CashDesk 
a credit card will appear above the register and a bot will give you some informations. 
Just drag that card to the register to confirm the purchase.

When you're ready to exit pass again through the leave cart line.

REVIEWER
Go and grab the item you want to review, this time don't pick the cart.
Enter into the review cabin holding the item in your hand , if all goes right the item should disappear 
and a UI appears allowing you to rate the item.
Now you can follow the instrucions to record a review: both the voice and the animation of your avatar
will be recorded and stored into the DB of that item.




