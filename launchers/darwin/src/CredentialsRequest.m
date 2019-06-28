#import "CredentialsRequest.h"
#import "Launcher.h"
#import "Settings.h"

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
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        NSLog(@"Credentials: request finished");
        NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
        NSInteger statusCode = [ne statusCode];
        NSLog(@"Credentials: Response status code: %ld", statusCode);
        NSLog(@"dante");
        if (!error) {
            if (statusCode == 200) {
                NSLog(@"--->");
                NSMutableData* webData = [NSMutableData data];
                [webData appendData:data];
                NSLog(@"---kdjf");
                NSString* jsonString = [[NSString alloc] initWithBytes: [webData mutableBytes] length:[data length] encoding:NSUTF8StringEncoding];
                NSData *jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
                NSError* jsonError;
                NSLog(@"fiest");
                id json = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&jsonError];
                NSLog(@"last");

                if (jsonError) {
                    NSLog(@"Credentials: Failed to parse json -> error: %@", jsonError);

                    dispatch_async(dispatch_get_main_queue(), ^{
                        Launcher* sharedLauncher = [Launcher sharedLauncher];
                        [sharedLauncher displayErrorPage];
                    });
                    return;
                }

                if (json[@"error"] != nil) {
                    NSLog(@"Credentials: Login failed -> error: %@", json[@"error"]);
                    dispatch_async(dispatch_get_main_queue(), ^{
                        Launcher* sharedLauncher = [Launcher sharedLauncher];
                        [[Settings sharedSettings] login:FALSE];
                        [sharedLauncher setLoginErrorState: CREDENTIALS];
                        [sharedLauncher credentialsAccepted:FALSE];
                    });
                } else {
                    NSLog(@"Credentials: Login Successful");
                    dispatch_async(dispatch_get_main_queue(), ^{
                         Launcher* sharedLauncher = [Launcher sharedLauncher];
                         [[Settings sharedSettings] login:TRUE];
                         [sharedLauncher setTokenString:jsonString];
                         [sharedLauncher credentialsAccepted:TRUE];
                    });
                }
            } else if (statusCode == 403 || statusCode == 404 || statusCode == 401) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    Launcher* sharedLauncher = [Launcher sharedLauncher];
                    [[Settings sharedSettings] login:FALSE];
                    [sharedLauncher setLoginErrorState: CREDENTIALS];
                    [sharedLauncher credentialsAccepted:FALSE];
                });
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    Launcher* sharedLauncher = [Launcher sharedLauncher];
                    [sharedLauncher displayErrorPage];
                });
            }
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
                Launcher* sharedLauncher = [Launcher sharedLauncher];
                [sharedLauncher displayErrorPage];
            });
        }
    }];

    [dataTask resume];
}
@end
