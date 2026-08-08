use std::collections::HashMap;
use std::f32::consts::PI;
use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::BufReader;
use std::os::raw::c_char;

use rbx_binary::from_reader;
use rbx_dom_weak::types::{BrickColor, CFrame, Color3uint8, Ref, Variant, Vector3};
use rbx_dom_weak::{Ustr, WeakDom};

#[repr(C)]
pub struct RbxlPartData {
    pub id: i64,
    pub parent_id: i64,
    pub is_container: bool,
    pub name: *const c_char,
    pub position: [f32; 3],
    pub size: [f32; 3],
    pub orientation: [f32; 3],
    pub color: [u8; 3],
    pub transparency: f32,
    pub anchored: bool,
    pub shape: RbxlPartType,
    pub is_spawn_location: bool
}

#[repr(C)]
pub enum RbxlPartType {
    Ball,
    Block,
    Cylinder,
    Wedge
}

fn is_exportable_class(class_name: &str) -> bool {
    matches!(class_name, "Model" | "Folder" | "Part" | "SpawnLocation")
}

fn resolve_parent_id(dom: &WeakDom, referent: Ref, id_map: &HashMap<Ref, i64>) -> i64 {
    let mut current = referent;
    
    while let Some(parent) = dom.get_by_ref(current).and_then(|inst| inst.parent().into()) {
        if parent == dom.root_ref() {
            break;
        }
        
        if let Some(&parent_id) = id_map.get(&parent) {
            return parent_id;
        }
        
        current = parent;
    }
    
    -1
}

#[no_mangle]
pub extern "C" fn rbxlLoad(path: *const c_char, out_count: *mut usize) -> *mut RbxlPartData {
    let c_path = unsafe { CStr::from_ptr(path) };
    let path_str = match c_path.to_str() {
        Ok(p) => p,
        Err(_) => return std::ptr::null_mut(),
    };

    let file = match File::open(path_str) {
        Ok(f) => f,
        Err(_) => return std::ptr::null_mut(),
    };

    let reader = BufReader::new(file);

    let dom: WeakDom = match from_reader(reader) {
        Ok(d) => d,
        Err(_) => return std::ptr::null_mut(),
    };

    let mut id_map: HashMap<Ref, i64> = HashMap::new();
    let mut next_id: i64 = 0;

    for inst in dom.descendants() {
        if is_exportable_class(inst.class.as_str()) {
            id_map.insert(inst.referent(), next_id);
            next_id += 1;
        }
    }

    let mut parts = Vec::new();

    for inst in dom.descendants() {
        let class = inst.class.as_str();
        let is_container = class == "Model" || class == "Folder";
        let is_part_like = class == "Part" || class == "SpawnLocation";

        if !is_container && !is_part_like {
            continue;
        }

        let id = *id_map
            .get(&inst.referent())
            .expect("exportable instance must have been assigned an id in the first pass");

        let parent_id = resolve_parent_id(&dom, inst.referent(), &id_map);

        let name = CString::new(inst.name.clone())
            .unwrap_or_else(|_| CString::new("Unknown").unwrap());

        if is_container {
            parts.push(RbxlPartData {
                id,
                parent_id,
                is_container: true,
                name: name.into_raw(),
                position: [0.0, 0.0, 0.0],
                size: [0.0, 0.0, 0.0],
                orientation: [0.0, 0.0, 0.0],
                color: [0, 0, 0],
                transparency: 0.0,
                anchored: false,
                shape: RbxlPartType::Block,
                is_spawn_location: false,
            });
            continue;
        }

        let is_spawn_location = class == "SpawnLocation";

        let (pos, ort) = match inst.properties.get(&Ustr::from("CFrame")) {
            Some(Variant::CFrame(cf)) => {
                let pos = cf.position;
                let rot = cframe_to_euler_rad(cf);
                (pos, rot)
            }
            _ => {
                let pos = inst
                    .properties
                    .get(&Ustr::from("Position"))
                    .and_then(|v| match v {
                        Variant::Vector3(v3) => Some(*v3),
                        _ => None,
                    })
                    .unwrap_or(Vector3::new(0.0, 0.0, 0.0));

                let rot = inst
                    .properties
                    .get(&Ustr::from("Orientation"))
                    .and_then(|v| match v {
                        Variant::Vector3(v3) => Some(*v3),
                        _ => None,
                    })
                    .unwrap_or(Vector3::new(0.0, 0.0, 0.0));

                (pos, rot)
            }
        };

        let size = inst
            .properties
            .get(&Ustr::from("Size"))
            .and_then(|v| match v {
                Variant::Vector3(v3) => Some(*v3),
                _ => None,
            })
            .unwrap_or(Vector3::new(4.0, 1.0, 2.0));

        let color = match inst.properties.get(&Ustr::from("Color")) {
            Some(Variant::Color3(c)) => Color3uint8 {
                r: (c.r * 255.0) as u8,
                g: (c.g * 255.0) as u8,
                b: (c.b * 255.0) as u8,
            },
            Some(Variant::BrickColor(br)) => br.to_color3uint8(),
            Some(Variant::Color3uint8(c)) => *c,
            _ => BrickColor::MediumStoneGrey.to_color3uint8(),
        };

        let transparency = inst
            .properties
            .get(&Ustr::from("Transparency"))
            .and_then(|v| match v {
                Variant::Float32(f) => Some(*f),
                _ => None,
            })
            .unwrap_or(0.0);

        let anchored = inst
            .properties
            .get(&Ustr::from("Anchored"))
            .and_then(|v| match v {
                Variant::Bool(b) => Some(*b),
                _ => None,
            })
            .unwrap_or(false);

        let shape_id = inst
            .properties
            .get(&Ustr::from("Shape"))
            .and_then(|v| match v {
                Variant::Enum(e) => Some(e.to_u32()),
                _ => None
            })
            .unwrap_or(1);

        let part_type = match shape_id {
            0 => RbxlPartType::Ball,
            1 => RbxlPartType::Block,
            2 => RbxlPartType::Cylinder,
            3 => RbxlPartType::Wedge,
            _ => RbxlPartType::Block,
        };

        parts.push(RbxlPartData {
            id,
            parent_id,
            is_container: false,
            name: name.into_raw(),
            position: [pos.x, pos.y, pos.z],
            size: [size.x, size.y, size.z],
            orientation: [ort.x, ort.y, ort.z],
            color: [color.r, color.g, color.b],
            transparency: transparency,
            anchored: anchored,
            shape: part_type,
            is_spawn_location: is_spawn_location
        });
    }

    unsafe {
        *out_count = parts.len();
    }

    let boxed = parts.into_boxed_slice();
    Box::into_raw(boxed) as *mut RbxlPartData
}

#[no_mangle]
pub extern "C" fn rbxlFree(ptr: *mut RbxlPartData, count: usize) {
    if ptr.is_null() {
        return;
    }
    unsafe {
        let slice = std::slice::from_raw_parts_mut(ptr, count);
        for item in slice.iter() {
            if !item.name.is_null() {
                let _ = CString::from_raw(item.name as *mut c_char);
            }
        }
        let _ = Box::from_raw(slice);
    }
}

fn cframe_to_euler_rad(cf: &CFrame) -> Vector3 {
    let orientation = cf.orientation;
    let m = [
        [orientation.x.x, orientation.x.y, orientation.x.z],
        [orientation.y.x, orientation.y.y, orientation.y.z],
        [orientation.z.x, orientation.z.y, orientation.z.z],
    ];

    let (x, y, z) = mat3_to_euler_zyx(m);
    Vector3::new(x, y, z)
}

fn mat3_to_euler_zyx(m: [[f32; 3]; 3]) -> (f32, f32, f32) {
    let m00 = m[0][0];
    let m10 = m[1][0];
    let m20 = m[2][0];
    let m21 = m[2][1];
    let m22 = m[2][2];

    let y = (-m20).clamp(-1.0, 1.0).asin();
    let (x, z);
    if y.abs() < (PI / 2.0 - 1e-4) {
        x = m21.atan2(m22);
        z = m10.atan2(m00);
    } else {
        x = 0.0;
        z = (-m[0][1]).atan2(m[1][1]);
    }

    (x, y, z)
}