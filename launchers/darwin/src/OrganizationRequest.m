#import "OrganizationRequest.h"
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#import "Launcher.h"


static NSString* const organizationURL = @"https://orgs.highfidelity.com/organizations/";

@interface OrganizationRequest ()
@property (nonatomic, assign) NSMutableData* receivedData;
@property (nonatomic, assign) NSInteger statusCode;
@end

@implementation OrganizationRequest

- (void) confirmOrganization:(NSString*)aOrganization :(NSString*)aUsername {
    self.username = aUsername;
    NSString* trimmedOrgString = [aOrganization stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    const char *cKey  = LAUNCHER_HMAC_SECRET;
    const char *cData = [[trimmedOrgString lowercaseString] cStringUsingEncoding:NSASCIIStringEncoding];
    unsigned char cHMAC[CC_SHA256_DIGEST_LENGTH];
    CCHmac(kCCHmacAlgSHA256, cKey, strlen(cKey), cData, strlen(cData), cHMAC);
    NSData *HMACData = [NSData dataWithBytes:cHMAC length:sizeof(cHMAC)];
    const unsigned char *buffer = (const unsigned char *)[HMACData bytes];
    NSMutableString *hash = [NSMutableString stringWithCapacity:HMACData.length * 2];
    for (int i = 0; i < HMACData.length; ++i){
        [hash appendFormat:@"%02x", buffer[i]];
    }

    NSString* jsonFile = [hash stringByAppendingString:@".json"];

    NSMutableURLRequest *request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString:[organizationURL stringByAppendingString:jsonFile]]];
    [request setHTTPMethod:@"GET"];
    [request setValue:@"" forHTTPHeaderField:@"Content-Type"];

    NSURLSession * session = [NSURLSession sessionWithConfiguration:NSURLSessionConfiguration.ephemeralSessionConfiguration delegate: self delegateQueue: [NSOperationQueue mainQueue]];
    NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request];
    /* NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
        NSInteger statusCode = [ne statusCode];
        NSLog(@"Organization Response status code: %ld", [ne statusCode]);

        if (!error) {
            if (statusCode == 200) {
                NSError *jsonError;
                NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&jsonError];
                if (jsonError) {
                    NSLog(@"Failed to parse Organzation json -> error: %@", jsonError);
                    dispatch_async(dispatch_get_main_queue(), ^{
                        Launcher* sharedLauncher = [Launcher sharedLauncher];
                        [sharedLauncher displayErrorPage];
                    });
                    return;
                }
                NSLog(@"getting org done");
                dispatch_async(dispatch_get_main_queue(), ^{
                    Launcher* sharedLauncher = [Launcher sharedLauncher];
                    [sharedLauncher setDomainURLInfo:[json valueForKey:@"domain"] :[json valueForKey:@"content_set_url"] :[json valueForKey:@"scripts_url"]];
                    [sharedLauncher setLoginErrorState: NONE];
                    [sharedLauncher organizationRequestFinished:TRUE];
                });
            } else if (statusCode == 403 || statusCode == 404) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    Launcher* sharedLauncher = [Launcher sharedLauncher];
                    [sharedLauncher setLoginErrorState: ORGANIZATION];
                    [sharedLauncher organizationRequestFinished:FALSE];
                });
            } else {
                NSLog(@ "Organization Response error: -> %@", error);
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
        }];*/

    [dataTask resume];
}


- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
    NSLog(@"----------------> ");

    self.receivedData = nil;
    self.receivedData = [[NSMutableData alloc] init];
    [self.receivedData setLength:0];
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    self.statusCode = [ne statusCode];
    NSLog(@"Organization Response status code: %ld", self.statusCode);
    completionHandler(NSURLSessionResponseAllow);
}


-(void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
   didReceiveData:(NSData *)data {

    [self.receivedData appendData:data];
    NSLog(@"did recieve data");
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    NSLog(@"did complete with error");
    if (error) {
        // Handle error
    }
    else {
        Launcher* sharedLauncher = [Launcher sharedLauncher];
        if (self.statusCode == 200) {
            NSError *jsonError;
            NSDictionary *json = [NSJSONSerialization JSONObjectWithData:self.receivedData options:kNilOptions error:&jsonError];
            if (jsonError) {
                NSLog(@"Failed to parse Organzation json -> error: %@", jsonError);
                [sharedLauncher displayErrorPage];
                return;
            }
            NSLog(@"getting org done");
            [sharedLauncher setDomainURLInfo:[json valueForKey:@"domain"] :[json valueForKey:@"content_set_url"] :[json valueForKey:@"scripts_url"]];
            [sharedLauncher setLoginErrorState: NONE];
            [sharedLauncher organizationRequestFinished:TRUE];
        } else if (self.statusCode == 403 || self.statusCode == 404) {
            [sharedLauncher setLoginErrorState: ORGANIZATION];
            [sharedLauncher organizationRequestFinished:FALSE];
        } else {
            NSLog(@ "Organization Response error: -> %@", error);
            [sharedLauncher displayErrorPage];
        }
    }
}

@end
