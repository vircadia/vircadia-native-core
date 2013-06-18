/*def targets = [
    'animation-server',
    'audio-mixer',
    'avatar-mixer',
    'domain-server',
    'eve',
    'interface',
    'pairing-server',
    'space-server',
    'voxel-server'
]*/

def targets = ['space-server']

def JENKINS_URL = 'https://jenkins.below92.com/'
def GITHUB_HOOK_URL = 'https://github.com/worklist/hifi/'
def GIT_REPO_URL = 'git@github.com:worklist/hifi.git'
def HIPCHAT_AUTH_TOKEN = '4ad6553471db605629852ff3265408'
def HIPCHAT_ROOM = 'High Fidelity'
def ARTIFACT_DESTINATION = 'a-tower.below92.com'

targets.each {
    def targetName = it
    
    job {
        name "hifi-${targetName}"
        logRotator(7, -1, -1, -1)
        
        scm {
            git(GIT_REPO_URL)
        }
       
        configure { project ->
            project / 'properties' << {
                'com.coravy.hudson.plugins.github.GithubProjectProperty' {
                    projectUrl GITHUB_HOOK_URL
                }
                
                'jenkins.plugins.hipchat.HipChatNotifier_-HipChatJobProperty' {
                    room HIPCHAT_ROOM
                }               
            }
            
            project / 'scm' << {
                includedRegions "${targetName}/.*\nlibraries/.*"
            }
            
            project / 'triggers' << 'com.cloudbees.jenkins.GitHubPushTrigger' {
                spec ''
            }
            
            project / 'publishers' << {
                'jenkins.plugins.hipchat.HipChatNotifier' {
                    jenkinsUrl JENKINS_URL
                    authToken HIPCHAT_AUTH_TOKEN
                    room HIPCHAT_ROOM
                }
                
                'jenkins.plugins.publish__over__ssh.BapSshPublisherPlugin' {
                    consolePrefix 'SSH: '
                    
                }
            }
            
            project / 'builders' << 'hudson.plugins.cmake.CmakeBuilder' {
                sourceDir targetName
                buildDir 'build'
                buildType 'Release'
                generator 'Unix Makefiles'
                makeCommand 'make'
                installCommand 'make install'
                projectCmakePath '/usr/bin/cmake'
                cleanBuild 'false'
                cleanInstallDir 'false'
            } 
        }
        
        publishers {
            publishScp(ARTIFACT_DESTINATION) {
                entry('**/build/$TARGET', '/deploy/$TARGET')
            }
        }
    }
}