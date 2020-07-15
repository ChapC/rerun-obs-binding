const OBSBinding = require('bindings')('RerunOBSBinding.node');

export interface OBSClient {
    init(video: VideoSettings, audio: AudioSettings, moduleConfigPath: string): boolean;
    loadModule(moduleBinaryPath: string, moduleDataPath: string) : void;
    setupEncoders(video: EncoderConfig, audio: EncoderConfig) : boolean;
    shutdown() : void;

    startStream(address: string, key: string) : void;
    stopStream() : void;

    getMainScene() : OBSScene;
}

export interface OBSScene {
    getName() : string;

    addSource(name: string, type: string, settings: OBSDataObject) : OBSSource;
    removeSource(source: OBSSource) : void;
}

export interface OBSSource {
    getName() : string;

    updateSettings(settings: OBSDataObject) : void;

    isEnabled() : boolean;
    setEnabled(enabled: boolean) : void;

    on(signal: string, callback: (data: any)=>void) : number;
    off(listenerId: number) : void;
}

export const enum SpeakerLayout {
    "Mono" = 1,
    "Stereo" = 2,
    "Surround51" = 6,
    "Surround71" = 8
}

export const enum GraphicsModule {
    Direct3D = "libobs-d3d11",
    OpenGL = "libobs-opengl"
}

export interface VideoSettings {
    module: GraphicsModule;
    fps: number;
    width: number;
    height: number;
}

export interface AudioSettings {
    samples: number;
    speakerLayout: SpeakerLayout;
}

export const enum VideoEncoderType { 
    x264 = 'obs_x264',
    NVENC = 'jim_nvenc',
    QuickSync = 'obs_qsv11'
}

export const enum AudioEncoderType {
    AAC = 'ffmpeg_aac',
    Opus = 'ffmpeg_opus'
}

export interface EncoderConfig {
    encoder: VideoEncoderType | AudioEncoderType;
    encoderSettings: OBSDataObject;
}

export enum OBSStatus {
    NOT_STARTED = 0, //The OBS process is not running
    INITIALIZED = 1, //OBS has started and the video/audio systems are active
    READY = 2, //Audio/video encoders and the RTMP output are configured
    STREAM_CONNECTING = 3, //Attempting to connect to the RTMP server
    STREAMING = 4 //Connected and streaming
}

export type OBSDataObject = {[key: string] : OBSDataValue};
type SupportedOBSType = string | number | boolean;
export interface OBSDataValue {
    type: string;
    value: SupportedOBSType | Array<OBSDataObject>;
}

export class OBSString implements OBSDataValue {
    type = 'string';
    constructor(public value: string){};
}

export class OBSInt implements OBSDataValue {
    type = 'int';
    constructor(public value: number){};
}

export class OBSDouble implements OBSDataValue {
    type = 'double';
    constructor(public value: number){};
}

export class OBSBool implements OBSDataValue {
    type = 'bool';
    constructor(public value: boolean){};
}

export class OBSArray implements OBSDataValue {
    type = 'array';
    constructor(public value: Array<OBSDataObject>){};
}

export default <OBSClient> OBSBinding;