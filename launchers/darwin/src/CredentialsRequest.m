#import "CredentialsRequest.h"
#import "Launcher.h"
#import "Settings.h"

@implementation CredentialsRequest

- (void) confirmCredentials:(NSString*)username :(NSString*)password {

    NSLog(@"web request started");
    NSString *post = [NSString stringWithFormat:@"grant_type=password&username=%@&password=%@&scope=owner",
        [username stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]],
        [password stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]]];
    NSData *postData = [post dataUsingEncoding:NSUTF8StringEncoding];
    NSString *postLength = [NSString stringWithFormat:@"%ld", (unsigned long)[postData length]];

    NSMutableURLRequest *request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString:@"https://metaverse.highfidelity.com/oauth/token"]];
    [request setHTTPMethod:@"POST"];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        
        NSLog(@"credentials request finished");
        NSMutableData* webData = [NSMutableData data];
        [webData appendData:data];
        NSString* jsonString = [[NSString alloc] initWithBytes: [webData mutableBytes] length:[data length] encoding:NSUTF8StringEncoding];
        NSData *jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
        id json = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
        
        Launcher* sharedLauncher = [Launcher sharedLauncher];
        if (json[@"error"] != nil) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [[Settings sharedSettings] login:FALSE];
                [sharedLauncher setLoginErrorState: CREDENTIALS];
                [sharedLauncher credentialsAccepted:FALSE];
            });
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
                [[Settings sharedSettings] login:TRUE];
                [sharedLauncher setTokenString:jsonString];
                [sharedLauncher credentialsAccepted:TRUE];
            });
        }
        
        NSLog(@"credentials: connectionDidFinished completed");
        
    }];
    
    [dataTask resume];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [self.webData appendData:data];
    NSLog(@"credentials connection received data");
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    NSLog(@"credentials connection received response");
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    if([ne statusCode] == 200) {
        NSLog(@"connection state is 200 - all okay");
    } else {
        NSLog(@"connection state is NOT 200");
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

-(void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    NSLog(@"Conn Err: %@", [error localizedDescription]);
    [[Launcher sharedLauncher] displayErrorPage];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    NSLog(@"credentials request finished");
    NSString* jsonString = [[NSString alloc] initWithBytes: [self.webData mutableBytes] length:[self.webData length] encoding:NSUTF8StringEncoding];
    NSData *data = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if (json[@"error"] != nil) {
        [[Settings sharedSettings] login:FALSE];
        [sharedLauncher setLoginErrorState: CREDENTIALS];
        [sharedLauncher credentialsAccepted:FALSE];
    } else {
        [[Settings sharedSettings] login:TRUE];
        [sharedLauncher setTokenString:jsonString];
        [sharedLauncher credentialsAccepted:TRUE];
    }
    
    NSLog(@"credentials: connectionDidFinished completed");
}

@end
