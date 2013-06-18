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
                'hudson.plugins.buildblocker.BuildBlockerProperty' {
                    useBuildBlocker true
                    blockingJobs 'hifi-dsl-seed'
                }         
            }
            
            project / 'scm' << {
                includedRegions "${targetName}/.*\nlibraries/.*"
            }
            
            project / 'triggers' << 'com.cloudbees.jenkins.GitHubPushTrigger' {
                spec ''
            }
            
            project / 'builders' << 'hudson.plugins.cmake.CmakeBuilder' {
                sourceDir targetName
                buildDir 'build'
                installDir ''
                buildType 'RelWithDebInfo'
                generator 'Unix Makefiles'
                makeCommand 'make'
                installCommand 'make install'
                preloadScript ''
                cmakeArgs ''
                projectCmakePath '/usr/bin/cmake'
                cleanBuild 'false'
                cleanInstallDir 'false'
                builderImpl ''
            }
        }
        
        publishers {            
            publishScp(ARTIFACT_DESTINATION) {
                entry("**/build/${targetName}", "deploy/${targetName}")
            }
        }
        
        configure { project ->
            project / 'publishers' << {
                'hudson.plugins.postbuildtask.PostbuildTask' {
                    'tasks' {
                        'hudson.plugins.postbuildtask.TaskProperties' {
                            logTexts {
                                'hudson.plugins.postbuildtask.LogProperties' {
                                    logText '.'
                                    operator 'AND'
                                }
                            }
                            EscalateStatus true
                            RunIfJobSuccessful true
                            script "curl -d 'action=deploy&role=highfidelity-live&revision=${targetName}' https://a-tower.below92.com"
                        }
                    }
                }
                
                'jenkins.plugins.hipchat.HipChatNotifier' {
                    jenkinsUrl JENKINS_URL
                    authToken HIPCHAT_AUTH_TOKEN
                    room HIPCHAT_ROOM
                }
            }
        }
    }
}