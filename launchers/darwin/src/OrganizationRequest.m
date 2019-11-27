#import "OrganizationRequest.h"
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#import "Launcher.h"
#import "Settings.h"


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
    [request setValue:@USER_AGENT_STRING forHTTPHeaderField:@"User-Agent"];
    [request setValue:@"" forHTTPHeaderField:@"Content-Type"];

    NSURLSession * session = [NSURLSession sessionWithConfiguration:NSURLSessionConfiguration.ephemeralSessionConfiguration delegate: self delegateQueue: [NSOperationQueue mainQueue]];
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
    NSLog(@"Organization Response status code: %ld", self.statusCode);
    completionHandler(NSURLSessionResponseAllow);
}


-(void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
   didReceiveData:(NSData *)data {

    [self.receivedData appendData:data];
    NSLog(@"Organization: did recieve data");
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if (error) {
        NSLog(@"Organization: Request completed with an error -> error: %@", error);
        [sharedLauncher displayErrorPage];
    } else {
        if (self.statusCode == 200) {
            NSError *jsonError = nil;
            NSDictionary *json = [NSJSONSerialization JSONObjectWithData:self.receivedData options:kNilOptions error:&jsonError];
            if (jsonError) {
                NSLog(@"Failed to parse Organzation json -> error: %@", jsonError);
                [sharedLauncher displayErrorPage];
                return;
            }
            NSString* domainURL = [json valueForKey:@"domain"];
            NSString* contentSetURL = [json valueForKey:@"content_set_url"];
            NSString* buildTag = [json valueForKey:@"build_tag"];
            if (buildTag == nil) {
                buildTag = @"";
            }

            if (domainURL != nil && contentSetURL != nil) {
                NSLog(@"Organization: getting org file successful");
                [sharedLauncher setDomainURLInfo:[json valueForKey:@"domain"] :[json valueForKey:@"content_set_url"] :nil];
                [sharedLauncher setLoginErrorState: NONE];
                [[Settings sharedSettings] setOrganizationBuildTag:buildTag];
                [sharedLauncher organizationRequestFinished:TRUE];
            } else {
                NSLog(@"Organization: Either domainURL: %@ or contentSetURL: %@ json entries are invalid", domainURL, contentSetURL);
                [sharedLauncher displayErrorPage];
            }
        } else if (self.statusCode == 403 || self.statusCode == 404) {
            NSLog(@"Organization: failed to get org file");
            [sharedLauncher setLoginErrorState: ORGANIZATION];
            [sharedLauncher organizationRequestFinished:FALSE];
        } else {
            NSLog(@ "Organization: Response error -> statusCode: %ld", self.statusCode);
            [sharedLauncher displayErrorPage];
        }
    }
}

@end
