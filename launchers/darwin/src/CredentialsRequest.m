#import "CredentialsRequest.h"
#import "Launcher.h"
#import "Settings.h"

@interface CredentialsRequest ()
@property (nonatomic, assign) NSMutableData* receivedData;
@property (nonatomic, assign) NSInteger statusCode;
@end

@implementation CredentialsRequest

- (void) confirmCredentials:(NSString*)username :(NSString*)password {

    NSLog(@"web request started");
    NSString* trimmedUsername = [username stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSString *post = [NSString stringWithFormat:@"grant_type=password&username=%@&password=%@&scope=owner",
        [trimmedUsername stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]],
        [password stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]]];
    NSData *postData = [post dataUsingEncoding:NSUTF8StringEncoding];
    NSString *postLength = [NSString stringWithFormat:@"%ld", (unsigned long)[postData length]];

    NSMutableURLRequest *request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString:@"https://metaverse.highfidelity.com/oauth/token"]];
    [request setHTTPMethod:@"POST"];
    [request setValue:@USER_AGENT_STRING forHTTPHeaderField:@"User-Agent"];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];

    NSURLSession* session = [NSURLSession sessionWithConfiguration:NSURLSessionConfiguration.ephemeralSessionConfiguration delegate: self delegateQueue: [NSOperationQueue mainQueue]];
    NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request];

    [dataTask resume];
}


- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
    self.receivedData = nil;
    self.receivedData = [[NSMutableData alloc] init];
    [self.receivedData setLength:0];
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    self.statusCode = [ne statusCode];
    NSLog(@"Credentials Response status code: %ld", self.statusCode);
    completionHandler(NSURLSessionResponseAllow);
}


-(void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
   didReceiveData:(NSData *)data {

    [self.receivedData appendData:data];
    NSLog(@"Credentials: did recieve data");
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if (error) {
        NSLog(@"Credentials: Request completed with an error -> error: %@", error);
        [sharedLauncher displayErrorPage];
    } else {
        if (self.statusCode == 200) {
            NSString* jsonString = [[NSString alloc] initWithBytes: [self.receivedData mutableBytes] length:[self.receivedData length] encoding:NSUTF8StringEncoding];
            NSData* jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
            NSError* jsonError = nil;
            id json = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&jsonError];

            if (jsonError) {
                NSLog(@"Credentials: Failed to parse json -> error: %@", jsonError);
                NSLog(@"Credentials: JSON string from data: %@", jsonString);
                [sharedLauncher displayErrorPage];
                return;
            }

            if (json[@"error"] != nil) {
                NSLog(@"Credentials: Login failed -> error: %@", json[@"error"]);
                [[Settings sharedSettings] login:FALSE];
                [sharedLauncher setLoginErrorState: CREDENTIALS];
                [sharedLauncher credentialsAccepted:FALSE];
            } else {
                NSLog(@"Credentials: Login succeeded");
                [[Settings sharedSettings] login:TRUE];
                [sharedLauncher setTokenString:jsonString];
                [sharedLauncher credentialsAccepted:TRUE];
            }
        } else if (self.statusCode == 403 || self.statusCode == 404 || self.statusCode == 401) {
            NSLog(@"Credentials: Log failed with statusCode: %ld", self.statusCode);
            [[Settings sharedSettings] login:FALSE];
            [sharedLauncher setLoginErrorState: CREDENTIALS];
            [sharedLauncher credentialsAccepted:FALSE];
        } else {
            [sharedLauncher displayErrorPage];
        }
    }
}
@end
