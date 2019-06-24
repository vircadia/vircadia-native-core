#import "OrganizationRequest.h"
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#import "Launcher.h"


static NSString* const organizationURL = @"https://orgs.highfidelity.com/organizations/";

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
    NSError *error;
    NSData *data = [NSData dataWithContentsOfURL: [NSURL URLWithString:[organizationURL stringByAppendingString:jsonFile]]];
    
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if (data) {
        NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
        [sharedLauncher setDomainURLInfo:[json valueForKey:@"domain"] :[json valueForKey:@"content_set_url"] :[json valueForKey:@"scripts_url"]];
        [sharedLauncher setLoginErrorState: NONE];
        return [sharedLauncher organizationRequestFinished:TRUE];
    }
    [sharedLauncher setLoginErrorState: ORGANIZATION];
    return [sharedLauncher organizationRequestFinished:FALSE];
    /*NSLog(@"URL: %@", [organizationURL stringByAppendingString:jsonFile]);
    NSMutableURLRequest *request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString: [organizationURL stringByAppendingString:@"High%20Fidelity"]]];
    [request setHTTPMethod:@"GET"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];

    NSURLConnection *theConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];

    if(theConnection) {
        self.webData = [NSMutableData data];
        NSLog(@"connection initiated");
    }*/
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [self.webData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    if([ne statusCode] == 200) {
        NSLog(@"connection state is 200 - all okay");
    } else {
        NSLog(@"connection state is NOT 200");
    }
}

-(void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    NSLog(@"Conn Err: %@", [error localizedDescription]);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    /*NSString* jsonString = [[NSString alloc] initWithBytes: [self.webData mutableBytes] length:[self.webData length] encoding:NSUTF8StringEncoding];
    NSData *data = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];*/
    
    /*Launcher* sharedLauncher = [Launcher sharedLauncher];
    if (json[@"error"] != nil) {
        NSLog(@"Login in failed");
        NSString* accessToken = [json objectForKey:@"access_token"];
        NSLog(@"access token %@", accessToken);
        [sharedLauncher credentialsAccepted:FALSE];
    } else {
        NSLog(@"Login successful");
        NSString* accessToken = [json objectForKey:@"access_token"];
        NSLog(@"access token %@", accessToken);
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        [defaults setValue:accessToken forKey:@"access_token"];
        [defaults synchronize];
        [sharedLauncher credentialsAccepted:TRUE];
    }*/
    //NSLog(@"OUTPUT:: %@", self.jsonString);
}

@end
