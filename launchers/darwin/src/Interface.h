#import <Foundation/Foundation.h>

@interface Interface : NSObject

-(id _Nonnull) initWith:(NSString * _Nonnull) aPathToInterface;
-(NSInteger) getVersion:(out NSError * _Nullable * _Nonnull) anError;

@end
